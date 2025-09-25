#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * ResolutionStore
 * Maintains an overlay of "method" -> "resolution object" (opaque JSON).
 * Last-wins overlay semantics per design.
 */
class ResolutionStore {
public:
    ResolutionStore() = default;
    ~ResolutionStore() = default;

    // PUBLIC_INTERFACE
    void Clear() {
        Core::SafeSyncType<Core::CriticalSection> lock(_lock);
        _store.clear();
    }

    // PUBLIC_INTERFACE
    void Overlay(const std::unordered_map<string, Core::JSON::Object>& addendum) {
        Core::SafeSyncType<Core::CriticalSection> lock(_lock);
        for (auto& kv : addendum) {
            _store[kv.first] = kv.second;
        }
    }

    // PUBLIC_INTERFACE
    bool Get(const string& appId, const string& method, const Core::JSON::Object& params, Core::JSON::Object& out) const {
        (void)appId;
        (void)params;
        Core::SafeSyncType<Core::CriticalSection> lock(_lock);
        auto it = _store.find(method);
        if (it != _store.end()) {
            out = it->second;
            return true;
        }
        return false;
    }

private:
    mutable Core::CriticalSection _lock;
    std::unordered_map<string, Core::JSON::Object> _store;
};

} // namespace Plugin
} // namespace WPEFramework
