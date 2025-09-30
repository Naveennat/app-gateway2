#pragma once

#include <core/core.h>
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

struct ProviderContext {
    string appId;
    string connectionId;
};

class ProviderRegistry {
public:
    ProviderRegistry() = default;
    ProviderRegistry(const ProviderRegistry&) = delete;
    ProviderRegistry& operator=(const ProviderRegistry&) = delete;

    // Register or update a provider for a capability.
    // If lastWins is true, the capability will be re-assigned to the new provider.
    // If lastWins is false and an entry exists, registration fails.
    bool Register(const string& capability, const ProviderContext& ctx, const bool lastWins, string& outError);

    // Unregister a provider capability. Only the owner (by appId) can unregister.
    bool Unregister(const string& capability, const string& ownerAppId, string& outError);

    // Resolve provider context for a capability.
    bool Resolve(const string& capability, ProviderContext& out) const;

    // Remove all entries belonging to a given appId (cleanup on app disconnect).
    void RemoveByApp(const string& appId);

private:
    mutable Core::CriticalSection _lock;
    std::unordered_map<string, ProviderContext> _byCapability;
};

} // namespace Plugin
} // namespace WPEFramework
