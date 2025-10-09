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

#include "Module.h"
#include <core/core.h>
#include <interfaces/IOttPermissionCache.h>

#include <map>
#include <mutex>
#include <string>
#include <vector>
#include <atomic>

namespace WPEFramework {
namespace Plugin {

    class OttPermissionCache : public Exchange::IOttPermissionCache {
    public:
        OttPermissionCache(const OttPermissionCache&) = delete;
        OttPermissionCache& operator=(const OttPermissionCache&) = delete;

        // PUBLIC_INTERFACE
        static OttPermissionCache& Instance();
        /** Singleton accessor.
         * @return Reference to the global OttPermissionCache instance.
         */

        // IUnknown reference counting (singleton-safe: never deletes the static instance)
        // PUBLIC_INTERFACE
        uint32_t AddRef() const override;
        /** Increase the reference count (no-op for lifetime as singleton). */

        // PUBLIC_INTERFACE
        uint32_t Release() const override;
        /** Decrease the reference count (no delete; singleton stays alive). */

        // BEGIN_INTERFACE_MAP provides QueryInterface; expose IOttPermissionCache and IUnknown
        BEGIN_INTERFACE_MAP(OttPermissionCache)
            INTERFACE_ENTRY(Exchange::IOttPermissionCache)
        END_INTERFACE_MAP

        // Exchange::IOttPermissionCache methods
        // PUBLIC_INTERFACE
        std::vector<string> GetPermissions(const string& appId) override;
        /** Get permissions for the specified appId.
         * Thread-safe. Returns a copy of the stored permissions vector.
         * @param appId Application identifier string.
         * @return Vector of permission strings or empty vector if none cached.
         */

        // PUBLIC_INTERFACE
        void UpdateCache(const string& appId, const std::vector<string>& permissions) override;
        /** Replace the cached permissions for appId with the provided list.
         * Thread-safe. Overwrites any existing entry.
         * @param appId Application identifier string.
         * @param permissions Vector of permission strings.
         */

        // PUBLIC_INTERFACE
        void Invalidate(const string& appId) override;
        /** Remove any cached permissions for appId.
         * Thread-safe. No-op if appId is not present.
         * @param appId Application identifier string.
         */

        // PUBLIC_INTERFACE
        void Clear() override;
        /** Clear the entire cache (maintenance utility).
         * Thread-safe. Removes all entries.
         */

        // PUBLIC_INTERFACE
        bool Has(const string& appId) const override;
        /** Check if cache contains an entry for appId.
         * Thread-safe.
         * @param appId Application identifier string.
         * @return true if an entry exists, false otherwise.
         */

        // PUBLIC_INTERFACE
        size_t Size() const override;
        /** Get current number of distinct appId entries cached.
         * Thread-safe.
         * @return Number of entries in cache.
         */

    private:
        OttPermissionCache() = default;
        ~OttPermissionCache() override = default;

    private:
        // Reference counter for COM-style lifetime management.
        // This class is a Meyers singleton; Release() will not delete the instance.
        mutable std::atomic<uint32_t> _refCount {1};

        // mutable to allow locking in const methods
        mutable std::mutex _admin;
        // In-memory cache; updates are performed only from non-const methods after acquiring the lock
        std::map<string, std::vector<string>> _cache;
    };

} // namespace Plugin
} // namespace WPEFramework
