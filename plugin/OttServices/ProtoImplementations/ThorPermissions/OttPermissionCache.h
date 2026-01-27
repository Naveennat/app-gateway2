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
//
// Cache policy:
// - Reads are strictly in-memory: GetPermissions() never consults disk.
// - Disk is only used for:
//    (1) one-time preload when the singleton is first used (Instance() when Size()==0)
//    (2) persistence during UpdateCache() (read-modify-write to preserve other apps).

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
        std::vector<string> GetPermissions(const string& appId);
        /** Get permissions for the specified appId.
         * Thread-safe. Returns a copy of the stored permissions vector.
         * @param appId Application identifier string.
         * @return Vector of permission strings or empty vector if none cached.
         */

        // PUBLIC_INTERFACE
        void UpdateCache(const string& appId, const std::vector<string>& permissions);
        /** Replace the cached permissions for appId with the provided list.
         * Thread-safe. Overwrites any existing entry in memory and performs an idempotent,
         * atomic on-disk update:
         *  - Reads existing file entries (if any), updates/replaces the target app's record,
         *  - Rewrites the file with a single entry per app (no duplicates),
         *  - Uses a temp file + rename for integrity.
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
        // In-memory cache; updates are performed only from non-const methods after acquiring the lock
        std::map<string, std::vector<string>> _cache;
    };

} // namespace Plugin
} // namespace WPEFramework
