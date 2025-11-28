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
#include "TokenCache.h"

#include <chrono>

namespace WPEFramework {
namespace Plugin {

    void TokenCache::Put(const std::string& key, const TokenEntry& entry) {
        std::lock_guard<std::mutex> guard(_lock);
        _cache[key] = entry;
    }

    bool TokenCache::Get(const std::string& key, std::string& outJson) const {
        std::lock_guard<std::mutex> guard(_lock);
        auto it = _cache.find(key);
        if (it == _cache.end()) {
            return false;
        }

        const auto& entry = it->second;

        if (entry.expiryEpochSec == 0) {
            // No expiry information; treat as not cacheable yet.
            return false;
        }

        const uint64_t nowSec =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
                                      std::chrono::system_clock::now().time_since_epoch())
                                      .count());

        // Consider expiring soon; refresh proactively.
        if (IsExpiringSoon(nowSec, entry.expiryEpochSec)) {
            return false;
        }

        outJson = entry.tokenJson;
        return !outJson.empty();
    }

    void TokenCache::Clear() {
        std::lock_guard<std::mutex> guard(_lock);
        _cache.clear();
    }

    bool TokenCache::IsExpiringSoon(uint64_t nowEpochSec, uint64_t expiryEpochSec) const {
        if (expiryEpochSec <= nowEpochSec) {
            return true;
        }
        const uint64_t delta = expiryEpochSec - nowEpochSec;
        return (delta <= kRefreshWindowSec);
    }

} // namespace Plugin
} // namespace WPEFramework