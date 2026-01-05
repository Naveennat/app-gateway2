/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#include "OttPermissionCache.h"

#include <fstream>
#include <sstream>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

namespace {
    // Default path to the on-disk permissions cache (line-delimited JSON, one record per app)
    static constexpr const char* kDefaultPermsFile = "/opt/app_thor_perms.json";

    // Resolve cache file path with environment override to support testing and deployment flexibility
    inline const std::string& PermsFilePath() {
        static std::string path;
        static bool initialized = false;
        if (!initialized) {
            const char* env = std::getenv("OTT_PERMS_FILE");
            path = (env && *env) ? std::string(env) : std::string(kDefaultPermsFile);
            initialized = true;
        }
        return path;
    }

    inline std::string LTrim(const std::string& s) {
        size_t i = 0;
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) { ++i; }
        return s.substr(i);
    }
    inline std::string RTrim(const std::string& s) {
        if (s.empty()) return s;
        size_t i = s.size();
        while (i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t' || s[i - 1] == '\r' || s[i - 1] == '\n')) { --i; }
        return s.substr(0, i);
    }
    inline std::string Trim(const std::string& s) {
        return RTrim(LTrim(s));
    }

    // Minimal escaping for JSON string values: escape backslashes and quotes
    inline std::string EscapeJSONString(const std::string& in) {
        std::string out;
        out.reserve(in.size());
        for (char c : in) {
            if (c == '\\' || c == '\"') {
                out.push_back('\\');
            }
            out.push_back(c);
        }
        return out;
    }

    // Minimal unescaping for JSON string values produced by EscapeJSONString
    inline std::string UnescapeJSONString(const std::string& in) {
        std::string out;
        out.reserve(in.size());
        bool escape = false;
        for (char c : in) {
            if (escape) {
                out.push_back(c);
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else {
                out.push_back(c);
            }
        }
        return out;
    }

    // Compose a single JSON line: {"appId":"...","permissions":["p1","p2",...]}
    inline std::string ComposePermsJsonLine(const std::string& appId, const std::vector<std::string>& perms) {
        std::ostringstream oss;
        oss << "{\"appId\":\"" << EscapeJSONString(appId) << "\",\"permissions\":[";
        for (size_t i = 0; i < perms.size(); ++i) {
            if (i != 0) {
                oss << ",";
            }
            oss << "\"" << EscapeJSONString(perms[i]) << "\"";
        }
        oss << "]}";
        return oss.str();
    }

    // Parse a single line of form: {"appId":"...","permissions":[ "...","...", ... ]}
    // Robustly parse the "appId" value by scanning ':' and the subsequent quoted string with escape handling.
    inline bool ParsePermsLine(const std::string& line, std::string& appIdOut, std::vector<std::string>& permsOut) {
        appIdOut.clear();
        permsOut.clear();

        const std::string keyAppId = "\"appId\"";
        size_t keyPos = line.find(keyAppId);
        if (keyPos == std::string::npos) {
            return false;
        }

        // Find the colon after "appId"
        size_t colon = line.find(':', keyPos + keyAppId.size());
        if (colon == std::string::npos) {
            return false;
        }

        // Find the opening quote of the value after the colon (skip whitespace)
        size_t pos = colon + 1;
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
            ++pos;
        }
        if (pos >= line.size() || line[pos] != '\"') {
            return false;
        }

        // Extract quoted value with escape handling
        size_t start = pos + 1;
        bool escape = false;
        size_t end = start;
        for (; end < line.size(); ++end) {
            char c = line[end];
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '\"') {
                break;
            }
        }
        if (end >= line.size()) {
            return false;
        }
        appIdOut = Trim(UnescapeJSONString(line.substr(start, end - start)));
        if (appIdOut.empty()) {
            // empty appId is not valid
            return false;
        }

        // Find permissions array
        const std::string keyPerms = "\"permissions\"";
        size_t ppos = line.find(keyPerms, end);
        if (ppos == std::string::npos) {
            return false;
        }
        size_t lbrack = line.find('[', ppos + keyPerms.size());
        if (lbrack == std::string::npos) {
            return false;
        }
        size_t rbrack = line.find(']', lbrack + 1);
        if (rbrack == std::string::npos) {
            return false;
        }

        // Parse quoted items within [ ... ]
        size_t cur = lbrack + 1;
        while (cur < rbrack) {
            // Skip whitespace and commas
            while (cur < rbrack && (line[cur] == ' ' || line[cur] == '\t' || line[cur] == ',')) {
                ++cur;
            }
            if (cur >= rbrack) break;
            if (line[cur] != '\"') {
                // Not a string entry, skip until next comma or end
                size_t next = line.find(',', cur);
                if (next == std::string::npos || next > rbrack) {
                    break;
                }
                cur = next + 1;
                continue;
            }
            // Extract quoted string
            size_t vstart = cur + 1;
            bool esc = false;
            size_t vend = vstart;
            for (; vend < rbrack; ++vend) {
                char c = line[vend];
                if (esc) {
                    esc = false;
                } else if (c == '\\') {
                    esc = true;
                } else if (c == '\"') {
                    break;
                }
            }
            if (vend >= rbrack) {
                break;
            }
            permsOut.emplace_back(UnescapeJSONString(line.substr(vstart, vend - vstart)));
            cur = vend + 1;
        }

        return true;
    }

    // Load latest permissions per appId from the file (last occurrence wins).
    // Skips invalid or empty appId entries.
    inline bool LoadLatestFromFile(std::map<std::string, std::vector<std::string>>& outMap) {
        outMap.clear();

        std::ifstream ifs(PermsFilePath());
        if (!ifs.is_open()) {
            return false;
        }

        std::string line;
        size_t lineNum = 0;
        size_t loaded = 0;
        while (std::getline(ifs, line)) {
            ++lineNum;
            // Skip empty/whitespace-only lines
            bool onlyWhitespace = true;
            for (char c : line) {
                if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                    onlyWhitespace = false;
                    break;
                }
            }
            if (onlyWhitespace) {
                continue;
            }

            std::string appId;
            std::vector<std::string> perms;
            if (ParsePermsLine(line, appId, perms)) {
                if (!appId.empty()) {
                    outMap[appId] = std::move(perms);
                    ++loaded;
                } else {
                    LOGWARN("OttPermissionCache: skipped empty appId in %s (line %zu)", PermsFilePath().c_str(), lineNum);
                }
            } else {
                LOGWARN("OttPermissionCache: skipped invalid JSON line in %s (line %zu)", PermsFilePath().c_str(), lineNum);
            }
        }

        return (loaded > 0);
    }

    // RAII file lock against a sidecar .lock file. Used to serialize read-modify-write cycles.
    struct ScopedFileLock {
        int fd;
        std::string path;
        bool ok;
        ScopedFileLock()
            : fd(-1), path(PermsFilePath() + ".lock"), ok(false) {
            fd = ::open(path.c_str(), O_CREAT | O_RDWR, 0644);
            if (fd >= 0) {
                if (::flock(fd, LOCK_EX) == 0) {
                    ok = true;
                } else {
                    LOGWARN("OttPermissionCache: flock(LOCK_EX) failed on %s (errno=%d)", path.c_str(), errno);
                }
            } else {
                LOGWARN("OttPermissionCache: open lock file failed %s (errno=%d)", path.c_str(), errno);
            }
        }
        ~ScopedFileLock() {
            if (fd >= 0) {
                (void)::flock(fd, LOCK_UN);
                ::close(fd);
            }
        }
    };

    // Atomically rewrite the entire cache file with provided entries:
    // - Writes to a temporary file first, then renames to target path.
    // - Ensures only one entry per appId exists.
    inline bool SaveAllAtomic(const std::map<std::string, std::vector<std::string>>& entries) {
        const std::string tmpPath = PermsFilePath() + ".tmp";
        {
            std::ofstream ofs(tmpPath, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) {
                LOGERR("OttPermissionCache: failed to open %s for write", tmpPath.c_str());
                return false;
            }
            for (const auto& kv : entries) {
                if (kv.first.empty()) {
                    LOGWARN("OttPermissionCache: skipping empty appId during save");
                    continue;
                }
                ofs << ComposePermsJsonLine(kv.first, kv.second) << '\n';
                if (!ofs.good()) {
                    LOGERR("OttPermissionCache: failed writing to %s", tmpPath.c_str());
                    return false;
                }
            }
            ofs.flush();
            if (!ofs.good()) {
                LOGERR("OttPermissionCache: flush failed for %s", tmpPath.c_str());
                return false;
            }
        }

        if (std::rename(tmpPath.c_str(), PermsFilePath().c_str()) != 0) {
            LOGERR("OttPermissionCache: rename(%s -> %s) failed (errno=%d)", tmpPath.c_str(), PermsFilePath().c_str(), errno);
            std::remove(tmpPath.c_str());
            return false;
        }
        LOGINFO("OttPermissionCache: atomically wrote %zu entries to %s", entries.size(), PermsFilePath().c_str());
        return true;
    }
} // anonymous

// PUBLIC_INTERFACE
OttPermissionCache& OttPermissionCache::Instance() {
    static OttPermissionCache g_instance;

    // On first use, attempt to pre-load the cache from disk.
    LOGINFO("OttPermissionCache: preloading cache from disk");
    if (g_instance.Size() == 0) {
        std::map<std::string, std::vector<std::string>> onDisk;
        if (LoadLatestFromFile(onDisk)) {
            LOGINFO("OttPermissionCache: loaded %zu entries from %s", onDisk.size(), PermsFilePath().c_str());
            std::lock_guard<std::mutex> lock(g_instance._admin);
            for (auto& kv : onDisk) {
                g_instance._cache[kv.first] = std::move(kv.second);
            }
            LOGINFO("OttPermissionCache: preloaded %zu entries from %s", g_instance._cache.size(), PermsFilePath().c_str());
        }
    }

    return g_instance;
}

// PUBLIC_INTERFACE
std::vector<std::string> OttPermissionCache::GetPermissions(const std::string& appId) {
    // Copy under lock into a local result to prevent reading shared state without protection
    std::vector<std::string> result;
    {
        std::lock_guard<std::mutex> lock(_admin);
        auto it = _cache.find(appId);
        if (it != _cache.end()) {
            result = it->second; // copy while holding the lock
        }
    }
    if (!result.empty()) {
        return result;
    }

    // If not found, try to refresh from disk (no lock held while doing I/O)
    std::map<std::string, std::vector<std::string>> onDisk;
    if (LoadLatestFromFile(onDisk)) {
        auto found = onDisk.find(appId);
        if (found != onDisk.end()) {
            // Store into cache under lock and return a local copy
            result = found->second;
            const size_t count = result.size();
            {
                std::lock_guard<std::mutex> lock(_admin);
                _cache[appId] = result;
            }
            LOGINFO("OttPermissionCache: cache miss recovered from %s for appId='%s' (count=%zu)",
                    PermsFilePath().c_str(), appId.c_str(), count);
            return result;
        }
        // Optionally, merge all onDisk to _cache for broader warmup under lock
        {
            std::lock_guard<std::mutex> lock(_admin);
            for (auto& kv : onDisk) {
                _cache.emplace(kv.first, std::move(kv.second));
            }
        }
        LOGWARN("OttPermissionCache: appId '%s' not present in %s after reload", appId.c_str(), PermsFilePath().c_str());
    } else {
        LOGWARN("OttPermissionCache: unable to load cache from %s (file missing or empty)", PermsFilePath().c_str());
    }

    return result; // empty when not found
}

// PUBLIC_INTERFACE
void OttPermissionCache::UpdateCache(const std::string& appId, const std::vector<std::string>& permissions) {
    // Guard: trim and validate appId (do not write empty)
    const std::string trimmed = Trim(appId);
    if (trimmed.empty()) {
        LOGWARN("OttPermissionCache: UpdateCache skipped due to empty appId");
        return;
    }

    // 1) Update in-memory cache under lock.
    {
        std::lock_guard<std::mutex> lock(_admin);
        _cache[trimmed] = permissions;
    }

    // 2) Serialize read-modify-write with a file lock for concurrency safety
    ScopedFileLock fileLock;

    // 3) Build an updated on-disk map (latest occurrence wins) and write atomically.
    std::map<std::string, std::vector<std::string>> all;
    // Load what's already there; even if this returns false, 'all' will contain any valid lines parsed.
    (void)LoadLatestFromFile(all);

    // Convert input types if needed and replace the app's entry (idempotent).
    std::vector<std::string> permsStd;
    permsStd.reserve(permissions.size());
    for (const auto& p : permissions) {
        permsStd.emplace_back(p);
    }
    all[trimmed] = std::move(permsStd);

    // Atomically rewrite the cache file to avoid duplicates and preserve integrity on crash/interrupt.
    if (SaveAllAtomic(all)) {
        LOGINFO("OttPermissionCache: updated cache for appId='%s' (entries=%zu)", trimmed.c_str(), all.size());
    } else {
        LOGERR("OttPermissionCache: failed to persist permissions for appId='%s' to %s",
               trimmed.c_str(), PermsFilePath().c_str());
    }
}

// PUBLIC_INTERFACE
void OttPermissionCache::Invalidate(const std::string& appId) {
    std::lock_guard<std::mutex> lock(_admin);
    auto it = _cache.find(appId);
    if (it != _cache.end()) {
        _cache.erase(it);
        LOGINFO("OttPermissionCache: invalidated appId='%s'", appId.c_str());
    } else {
        LOGWARN("OttPermissionCache: invalidate requested for non-existent appId='%s'", appId.c_str());
    }
}

// PUBLIC_INTERFACE
void OttPermissionCache::Clear() {
    std::lock_guard<std::mutex> lock(_admin);
    const size_t before = _cache.size();
    _cache.clear();
    LOGINFO("OttPermissionCache: cleared all cached entries (previously %zu)", before);
}

// PUBLIC_INTERFACE
bool OttPermissionCache::Has(const std::string& appId) const {
    std::lock_guard<std::mutex> lock(_admin);
    return _cache.find(appId) != _cache.end();
}

// PUBLIC_INTERFACE
size_t OttPermissionCache::Size() const {
    std::lock_guard<std::mutex> lock(_admin);
    return _cache.size();
}

} // namespace Plugin
} // namespace WPEFramework
