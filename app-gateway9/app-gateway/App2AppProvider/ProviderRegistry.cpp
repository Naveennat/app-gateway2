#include "Module.h"
#include "ProviderRegistry.h"

namespace WPEFramework {
namespace Plugin {

bool ProviderRegistry::Register(const string& capability, const ProviderContext& ctx, const bool lastWins, string& outError) {
    outError.clear();

    if (capability.empty() || ctx.appId.empty() || ctx.connectionId.empty()) {
        outError = _T("INVALID_PARAMS");
        return false;
    }

    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    auto it = _byCapability.find(capability);
    if (it == _byCapability.end()) {
        _byCapability.emplace(capability, ctx);
        return true;
    }

    if (lastWins) {
        it->second = ctx;
        return true;
    }

    outError = _T("DUPLICATE_PROVIDER");
    return false;
}

bool ProviderRegistry::Unregister(const string& capability, const string& ownerAppId, string& outError) {
    outError.clear();

    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    auto it = _byCapability.find(capability);
    if (it == _byCapability.end()) {
        outError = _T("UNKNOWN_CAPABILITY");
        return false;
    }
    if (it->second.appId != ownerAppId) {
        outError = _T("NOT_OWNER");
        return false;
    }

    _byCapability.erase(it);
    return true;
}

bool ProviderRegistry::Resolve(const string& capability, ProviderContext& out) const {
    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    auto it = _byCapability.find(capability);
    if (it == _byCapability.end()) {
        return false;
    }
    out = it->second;
    return true;
}

void ProviderRegistry::RemoveByApp(const string& appId) {
    Core::SafeSyncType<Core::CriticalSection> guard(_lock);
    for (auto it = _byCapability.begin(); it != _byCapability.end();) {
        if (it->second.appId == appId) {
            it = _byCapability.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace Plugin
} // namespace WPEFramework
