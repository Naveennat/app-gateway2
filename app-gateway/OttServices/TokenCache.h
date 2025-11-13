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
