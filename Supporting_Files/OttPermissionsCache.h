#pragma once

// OttPermissionsCache.h
// Minimal, header-only in-memory cache used by the OttServices plugin to store
// and retrieve permissions per application. This is a lightweight stand-in for
// a more sophisticated shared cache that may exist in other repositories.

#include <core/core.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace WPEFramework {
namespace Plugin {

    class OttPermissionCache {
    public:
        OttPermissionCache(const OttPermissionCache&) = delete;
        OttPermissionCache& operator=(const OttPermissionCache&) = delete;

        // PUBLIC_INTERFACE
        static OttPermissionCache& Instance()
        {
            static OttPermissionCache g_instance;
            return g_instance;
        }

        // PUBLIC_INTERFACE
        std::vector<string> GetPermissions(const string& appId)
        {
            std::lock_guard<std::mutex> lock(_admin);
            auto it = _cache.find(appId);
            if (it != _cache.end()) {
                return it->second;
            }
            return std::vector<string>();
        }

        // PUBLIC_INTERFACE
        void UpdateCache(const string& appId, const std::vector<string>& permissions)
        {
            std::lock_guard<std::mutex> lock(_admin);
            _cache[appId] = permissions;
        }

        // PUBLIC_INTERFACE
        void Invalidate(const string& appId)
        {
            std::lock_guard<std::mutex> lock(_admin);
            _cache.erase(appId);
        }

    private:
        OttPermissionCache() = default;
        ~OttPermissionCache() = default;

    private:
        std::mutex _admin;
        std::map<string, std::vector<string>> _cache;
    };

} // namespace Plugin
} // namespace WPEFramework
