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

// TokenCache: simple in-memory token cache scaffold.
// Not wired into flows yet. Provided for future token retrieval path.

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

    struct TokenEntry {
        std::string tokenJson;
        uint64_t    expiryEpochSec {0}; // 0 indicates unknown/never cached
    };

    class TokenCache {
    public:
        TokenCache() = default;
        ~TokenCache() = default;

        // PUBLIC_INTERFACE
        void Put(const std::string& key, const TokenEntry& entry);

        // PUBLIC_INTERFACE
        // Returns true if a non-expiring or not-yet-expired entry exists.
        // outJson will be set to the cached entry's tokenJson; returns false otherwise.
        bool Get(const std::string& key, std::string& outJson) const;

        // PUBLIC_INTERFACE
        void Clear();

    private:
        bool IsExpiringSoon(uint64_t nowEpochSec, uint64_t expiryEpochSec) const;

    private:
        // Refresh-before-expiry window in seconds (placeholder).
        static constexpr uint64_t kRefreshWindowSec = 60;

        mutable std::mutex _lock;
        std::unordered_map<std::string, TokenEntry> _cache;
    };

} // namespace Plugin
} // namespace WPEFramework

