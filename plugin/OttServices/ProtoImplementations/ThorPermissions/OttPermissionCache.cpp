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

#include <core/JSON.h>

#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <fcntl.h>
#include <fstream>
#include <sys/file.h>
#include <unistd.h>

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

namespace {
    // Default path to the on-disk permissions cache (line-delimited JSON, one record per app).
    static constexpr const char* kDefaultPermsFile = "/opt/app_thor_perms.json";

    // Resolve cache file path with environment override to support testing and deployment flexibility.
    inline const std::string& PermsFilePath()
    {
        static std::string path;
        static bool initialized = false;

        if (initialized == false) {
            const char* env = std::getenv("OTT_PERMS_FILE");
            path = (env && *env) ? std::string(env) : std::string(kDefaultPermsFile);
            initialized = true;
        }
        return path;
    }

    inline std::string LTrim(const std::string& s)
    {
        size_t i = 0;
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
            ++i;
        }
        return s.substr(i);
    }

    inline std::string RTrim(const std::string& s)
    {
        if (s.empty()) {
            return s;
        }
        size_t i = s.size();
        while (i > 0 && (s[i - 1] == ' ' || s[i - 1] == '\t' || s[i - 1] == '\r' || s[i - 1] == '\n')) {
            --i;
        }
        return s.substr(0, i);
    }

    inline std::string Trim(const std::string& s)
    {
        return RTrim(LTrim(s));
    }

    inline bool IsWhitespaceOnly(const std::string& s)
    {
        for (char c : s) {
            if (!(c == ' ' || c == '\t' || c == '\r' || c == '\n')) {
                return false;
            }
        }
        return true;
    }

    // A single on-disk record (one JSON object per line):
    //   {"appId":"...","permissions":["p1","p2",...]}
    class PermissionRecord final : public Core::JSON::Container {
    public:
        PermissionRecord()
            : Core::JSON::Container()
            , AppId()
            , Permissions()
        {
            Add(_T("appId"), &AppId);
            Add(_T("permissions"), &Permissions);
        }

        Core::JSON::String AppId;
        Core::JSON::ArrayType<Core::JSON::String> Permissions;
    };

    inline bool ValidateRecord(const PermissionRecord& record)
    {
        // appId must exist and be non-empty (post-trim)
        if (record.AppId.IsSet() == false) {
            return false;
        }
        const std::string appId = Trim(record.AppId.Value());
        if (appId.empty()) {
            return false;
        }

        // permissions may be empty, but the field must be present
        if (record.Permissions.IsSet() == false) {
            return false;
        }

        return true;
    }

    inline void RecordToEntry(const PermissionRecord& record, std::string& appIdOut, std::vector<std::string>& permsOut)
    {
        appIdOut = Trim(record.AppId.Value());

        permsOut.clear();

        // Thunder Core::JSON ArrayType does not always support STL begin/end iteration.
        // Use the Thunder iterator pattern: Elements().Next()/Current().
        auto it = record.Permissions.Elements();
        while (it.Next() == true) {
            const auto& p = it.Current();
            permsOut.emplace_back(p.Value());
        }
    }

    inline void EntryToRecord(const std::string& appId, const std::vector<std::string>& perms, PermissionRecord& record)
    {
        record.AppId = appId;

        record.Permissions.Clear();
        for (const auto& p : perms) {
            Core::JSON::String element;
            element = p;
            record.Permissions.Add(element);
        }
    }

    // Load latest permissions per appId from the file (last occurrence wins).
    // Skips invalid JSON, missing fields, or empty appId entries.
    inline bool LoadLatestFromFile(std::map<std::string, std::vector<std::string>>& outMap)
    {
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

            if (line.empty() || IsWhitespaceOnly(line)) {
                continue;
            }

            PermissionRecord record;
            Core::OptionalType<Core::JSON::Error> error;

            if (record.FromString(line, error) == false || error.IsSet() == true) {
                LOGWARN("OttPermissionCache: skipped invalid JSON line in %s (line %zu)", PermsFilePath().c_str(), lineNum);
                continue;
            }

            if (ValidateRecord(record) == false) {
                LOGWARN("OttPermissionCache: skipped invalid permission record in %s (line %zu)", PermsFilePath().c_str(), lineNum);
                continue;
            }

            std::string appId;
            std::vector<std::string> perms;
            RecordToEntry(record, appId, perms);

            // last occurrence wins
            outMap[appId] = std::move(perms);
            ++loaded;
        }

        return (loaded > 0);
    }

    // RAII file lock against a sidecar .lock file. Used to serialize read-modify-write cycles.
    struct ScopedFileLock {
        int fd;
        std::string path;
        bool ok;

        ScopedFileLock()
            : fd(-1)
            , path(PermsFilePath() + ".lock")
            , ok(false)
        {
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

        ~ScopedFileLock()
        {
            if (fd >= 0) {
                (void)::flock(fd, LOCK_UN);
                ::close(fd);
            }
        }
    };

    // Atomically rewrite the entire cache file with provided entries:
    // - Writes to a temporary file first, then renames to target path.
    // - Ensures only one entry per appId exists (map keys).
    inline bool SaveAllAtomic(const std::map<std::string, std::vector<std::string>>& entries)
    {
        const std::string tmpPath = PermsFilePath() + ".tmp";
        {
            std::ofstream ofs(tmpPath, std::ios::out | std::ios::trunc);
            if (!ofs.is_open()) {
                LOGERR("OttPermissionCache: failed to open %s for write", tmpPath.c_str());
                return false;
            }

            // One JSON object per line.
            for (const auto& kv : entries) {
                if (kv.first.empty()) {
                    LOGWARN("OttPermissionCache: skipping empty appId during save");
                    continue;
                }

                PermissionRecord record;
                EntryToRecord(kv.first, kv.second, record);

                std::string jsonLine;
                const bool serialized = record.ToString(jsonLine);
                if ((serialized == false) || jsonLine.empty()) {
                    LOGERR("OttPermissionCache: failed to serialize record for appId='%s'", kv.first.c_str());
                    return false;
                }

                ofs << jsonLine << '\n';
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

        // Atomic replacement.
        if (std::rename(tmpPath.c_str(), PermsFilePath().c_str()) != 0) {
            LOGERR("OttPermissionCache: rename(%s -> %s) failed (errno=%d)", tmpPath.c_str(), PermsFilePath().c_str(), errno);
            std::remove(tmpPath.c_str());
            return false;
        }

        LOGINFO("OttPermissionCache: atomically wrote %zu entries to %s", entries.size(), PermsFilePath().c_str());
        return true;
    }
} // anonymous namespace

// PUBLIC_INTERFACE
OttPermissionCache& OttPermissionCache::Instance()
{
    static OttPermissionCache g_instance;

    // On first use, attempt to pre-load the cache from disk.
    if (g_instance.Size() == 0) {
        LOGINFO("OttPermissionCache: preloading cache from disk");
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
std::vector<string> OttPermissionCache::GetPermissions(const string& appId)
{
    // Copy under lock into a local result to prevent reading shared state without protection
    std::vector<string> result;
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
void OttPermissionCache::UpdateCache(const string& appId, const std::vector<string>& permissions)
{
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

    // 2) Serialize read-modify-write with a file lock for concurrency safety.
    LOGINFO("OttPermissionCache: attempting to acquire file lock for appId='%s'", trimmed.c_str());
    ScopedFileLock fileLock;
    LOGINFO("OttPermissionCache: file lock acquired for appId='%s'", trimmed.c_str());

    // 3) Build an updated on-disk map (latest occurrence wins) and write atomically.
    std::map<std::string, std::vector<std::string>> all;
    // Load what's already there; even if this returns false, 'all' will contain any valid lines parsed.
    (void)LoadLatestFromFile(all);

    // Replace the app's entry (idempotent).
    all[trimmed] = permissions;

    // Atomically rewrite the cache file to avoid duplicates and preserve integrity on crash/interrupt.
    if (SaveAllAtomic(all)) {
        LOGINFO("OttPermissionCache: updated cache for appId='%s' (entries=%zu)", trimmed.c_str(), all.size());
    } else {
        LOGERR("OttPermissionCache: failed to persist permissions for appId='%s' to %s",
            trimmed.c_str(), PermsFilePath().c_str());
    }
}

// PUBLIC_INTERFACE
void OttPermissionCache::Invalidate(const string& appId)
{
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
void OttPermissionCache::Clear()
{
    std::lock_guard<std::mutex> lock(_admin);
    const size_t before = _cache.size();
    _cache.clear();
    LOGINFO("OttPermissionCache: cleared all cached entries (previously %zu)", before);
}

// PUBLIC_INTERFACE
bool OttPermissionCache::Has(const string& appId) const
{
    std::lock_guard<std::mutex> lock(_admin);
    return _cache.find(appId) != _cache.end();
}

// PUBLIC_INTERFACE
size_t OttPermissionCache::Size() const
{
    std::lock_guard<std::mutex> lock(_admin);
    return _cache.size();
}

} // namespace Plugin
} // namespace WPEFramework
