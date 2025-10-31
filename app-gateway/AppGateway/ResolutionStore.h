#pragma once

/*
 * Minimal ResolutionStore used by unit tests to verify overlay and retrieval behavior.
 * Stores resolution objects keyed by a string. Later overlays override earlier entries (last-wins).
 */

#include <core/JSON.h>
#include <unordered_map>
#include <string>

namespace WPEFramework {
namespace Plugin {

class ResolutionStore {
public:
    ResolutionStore() = default;
    ~ResolutionStore() = default;

    // PUBLIC_INTERFACE
    bool Get(const std::string& /*appId*/, const std::string& key, const Core::JSON::Object& /*params*/, Core::JSON::Object& out) const
    {
        // Retrieve stored resolution by key; ignore appId/params as tests don't require them.
        auto it = _store.find(key);
        if (it == _store.end()) {
            return false;
        }
        out = it->second; // copy object
        return true;
    }

    // PUBLIC_INTERFACE
    void Overlay(const std::unordered_map<std::string, Core::JSON::Object>& additions)
    {
        // Apply overlay; last-wins semantics by overwriting existing keys.
        for (const auto& kv : additions) {
            _store[kv.first] = kv.second;
        }
    }

private:
    std::unordered_map<std::string, Core::JSON::Object> _store;
};

} // namespace Plugin
} // namespace WPEFramework
