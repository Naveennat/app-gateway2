#include "OttPermissionCache.h"

#include <fstream>
#include <sstream>
#include <cstdio>

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

namespace {
    // Path to the on-disk permissions cache (line-delimited JSON, one record per app update)
    static constexpr const char* kPermsFile = "/opt/app_perms.json";

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

    // Append a single JSON line: {"appId":"...","permissions":["p1","p2",...]}
    inline bool AppendPermsLine(const std::string& appId, const std::vector<std::string>& perms) {
        std::ofstream ofs(kPermsFile, std::ios::out | std::ios::app);
        if (!ofs.is_open()) {
            LOGERR("OttPermissionCache: failed to open %s for append", kPermsFile);
            return false;
        }

        std::ostringstream oss;
        oss << "{\"appId\":\"" << EscapeJSONString(appId) << "\",\"permissions\":[";
        for (size_t i = 0; i < perms.size(); ++i) {
            if (i != 0) {
                oss << ",";
            }
            oss << "\"" << EscapeJSONString(perms[i]) << "\"";
        }
        oss << "]}";

        ofs << oss.str() << '\n';
        if (!ofs.good()) {
            LOGERR("OttPermissionCache: write to %s failed", kPermsFile);
            return false;
        }

        return true;
    }

    // Parse a single line of form: {"appId":"...","permissions":[ "...","...", ... ]}
    inline bool ParsePermsLine(const std::string& line, std::string& appIdOut, std::vector<std::string>& permsOut) {
        appIdOut.clear();
        permsOut.clear();

        // Find "appId"
        const std::string keyAppId = "\"appId\"";
        size_t pos = line.find(keyAppId);
        if (pos == std::string::npos) {
            return false;
        }
        pos = line.find('"', pos + keyAppId.size());
        if (pos == std::string::npos) {
            return false;
        }
        // Next quote starts the value
        pos = line.find('"', pos + 1);
        if (pos == std::string::npos) {
            return false;
        }
        size_t start = pos + 1;

        // Find end quote (handle escapes)
        bool escape = false;
        size_t end = start;
        for (; end < line.size(); ++end) {
            char c = line[end];
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                break;
            }
        }
        if (end >= line.size()) {
            return false;
        }
        appIdOut = UnescapeJSONString(line.substr(start, end - start));

        // Find permissions array
        const std::string keyPerms = "\"permissions\"";
        size_t ppos = line.find(keyPerms, end);
        if (ppos == std::string::npos) {
            // No permissions key
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
            if (line[cur] != '"') {
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
                } else if (c == '"') {
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

    // Load latest permissions per appId from the file (last occurrence wins)
    inline bool LoadLatestFromFile(std::map<std::string, std::vector<std::string>>& outMap) {
        outMap.clear();

        std::ifstream ifs(kPermsFile);
        if (!ifs.is_open()) {
            return false;
        }

        std::string line;
        size_t lineNum = 0;
        size_t loaded = 0;
        while (std::getline(ifs, line)) {
            ++lineNum;
            // Skip empty lines
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
                outMap[appId] = std::move(perms);
                ++loaded;
            } else {
                LOGWARN("OttPermissionCache: skipped invalid JSON line in %s (line %zu)", kPermsFile, lineNum);
            }
        }

        return (loaded > 0);
    }
} // anonymous

// PUBLIC_INTERFACE
OttPermissionCache& OttPermissionCache::Instance() {
    static OttPermissionCache g_instance;

    // On first use, attempt to pre-load the cache from disk.
    if (g_instance.Size() == 0) {
        std::map<std::string, std::vector<std::string>> onDisk;
        if (LoadLatestFromFile(onDisk)) {
            std::lock_guard<std::mutex> lock(g_instance._admin);
            for (auto& kv : onDisk) {
                g_instance._cache[kv.first] = std::move(kv.second);
            }
            LOGINFO("OttPermissionCache: preloaded %zu entries from %s", g_instance._cache.size(), kPermsFile);
        }
    }

    return g_instance;
}

 // PUBLIC_INTERFACE
std::vector<string> OttPermissionCache::GetPermissions(const string& appId) {
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
                    kPermsFile, appId.c_str(), count);
            return result;
        }
        // Optionally, merge all onDisk to _cache for broader warmup under lock
        {
            std::lock_guard<std::mutex> lock(_admin);
            for (auto& kv : onDisk) {
                _cache.emplace(kv.first, kv.second);
            }
        }
        LOGWARN("OttPermissionCache: appId '%s' not present in %s after reload", appId.c_str(), kPermsFile);
    } else {
        LOGWARN("OttPermissionCache: unable to load cache from %s (file missing or empty)", kPermsFile);
    }

    return result; // empty when not found
}

// PUBLIC_INTERFACE
void OttPermissionCache::UpdateCache(const string& appId, const std::vector<string>& permissions) {
    {
        std::lock_guard<std::mutex> lock(_admin);
        _cache[appId] = permissions;
    }

    // Append this update to the on-disk log for persistence
    if (AppendPermsLine(appId, permissions)) {
        LOGINFO("OttPermissionCache: updated in-memory and appended %zu permissions for appId='%s' to %s",
                permissions.size(), appId.c_str(), kPermsFile);
    } else {
        LOGERR("OttPermissionCache: failed to append permissions for appId='%s' to %s",
               appId.c_str(), kPermsFile);
    }
}

// PUBLIC_INTERFACE
void OttPermissionCache::Invalidate(const string& appId) {
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
bool OttPermissionCache::Has(const string& appId) const {
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
