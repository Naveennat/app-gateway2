#pragma once

#include "Module.h"
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

/**
 * ProviderRegistry
 * Tracks capability -> provider context.
 * Only one provider is allowed per capability (policy: lastWins or rejectDuplicates).
 */
class ProviderRegistry {
public:
    struct ProviderContext {
        string appId;
        string connectionId;
    };

public:
    ProviderRegistry()
        : _lastWins(true) {
    }

    void SetLastWinsPolicy(const bool lastWins) {
        _lastWins = lastWins;
    }

    // PUBLIC_INTERFACE
    /**
     * Register a provider for a capability.
     * Returns true on success. If a provider already exists:
     *  - lastWins=true: overwrites with the new provider and returns true
     *  - lastWins=false: does not change and returns false
     */
    bool Register(const string& capability, const ProviderContext& ctx) {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);

        auto it = _byCapability.find(capability);
        if (it == _byCapability.end()) {
            _byCapability.emplace(capability, ctx);
            return true;
        }

        if (_lastWins) {
            it->second = ctx;
            return true;
        }

        return false;
    }

    // PUBLIC_INTERFACE
    /**
     * Unregister a provider for a capability; only the owning app can unregister.
     * Returns true if unregistered; false if not found or not owner.
     */
    bool Unregister(const string& capability, const string& appId) {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);

        auto it = _byCapability.find(capability);
        if (it == _byCapability.end()) {
            return false;
        }
        if (it->second.appId != appId) {
            return false;
        }
        _byCapability.erase(it);
        return true;
    }

    // PUBLIC_INTERFACE
    /**
     * Resolve a capability to a ProviderContext.
     * Returns true if found and writes out, false otherwise.
     */
    bool Resolve(const string& capability, ProviderContext& out) const {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);

        auto it = _byCapability.find(capability);
        if (it == _byCapability.end()) {
            return false;
        }
        out = it->second;
        return true;
    }

    // PUBLIC_INTERFACE
    /**
     * Clear all mappings (used on deinitialize).
     */
    void Clear() {
        Core::SafeSyncType<Core::CriticalSection> guard(_lock);
        _byCapability.clear();
    }

private:
    bool _lastWins;
    mutable Core::CriticalSection _lock;
    std::unordered_map<string, ProviderContext> _byCapability;
};

} // namespace Plugin
} // namespace WPEFramework
