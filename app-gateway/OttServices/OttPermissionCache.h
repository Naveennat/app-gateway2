#pragma once

// OttPermissionCache.h
// Thread-safe in-memory permission cache for OTT services.
// Stores permissions keyed by application ID.
//
// Typical usage within OttServices:
// - UpdateCache(appId, permissions) after fetching from remote permission service.
// - GetPermissions(appId) to retrieve cached permissions.
// - Invalidate(appId) to drop cached entry for an app.
// - Clear() to drop all cached entries (maintenance utility).

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
        static OttPermissionCache& Instance();
        /** Singleton accessor.
         * @return Reference to the global OttPermissionCache instance.
         */

        // PUBLIC_INTERFACE
        std::vector<string> GetPermissions(const string& appId) const;
        /** Get permissions for the specified appId.
         * Thread-safe. Returns a copy of the stored permissions vector.
         * @param appId Application identifier string.
         * @return Vector of permission strings or empty vector if none cached.
         */

        // PUBLIC_INTERFACE
        void UpdateCache(const string& appId, const std::vector<string>& permissions);
        /** Replace the cached permissions for appId with the provided list.
         * Thread-safe. Overwrites any existing entry.
         * @param appId Application identifier string.
         * @param permissions Vector of permission strings.
         */

        // PUBLIC_INTERFACE
        void Invalidate(const string& appId);
        /** Remove any cached permissions for appId.
         * Thread-safe. No-op if appId is not present.
         * @param appId Application identifier string.
         */

        // PUBLIC_INTERFACE
        void Clear();
        /** Clear the entire cache (maintenance utility).
         * Thread-safe. Removes all entries.
         */

        // PUBLIC_INTERFACE
        bool Has(const string& appId) const;
        /** Check if cache contains an entry for appId.
         * Thread-safe.
         * @param appId Application identifier string.
         * @return true if an entry exists, false otherwise.
         */

        // PUBLIC_INTERFACE
        size_t Size() const;
        /** Get current number of distinct appId entries cached.
         * Thread-safe.
         * @return Number of entries in cache.
         */

    private:
        OttPermissionCache() = default;
        ~OttPermissionCache() = default;

    private:
        // mutable to allow locking in const methods
        mutable std::mutex _admin;
        std::map<string, std::vector<string>> _cache;
    };

} // namespace Plugin
} // namespace WPEFramework
