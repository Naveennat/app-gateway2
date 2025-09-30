#pragma once

#include <core/core.h>

namespace WPEFramework {
namespace Plugin {

class PermissionManager {
public:
    explicit PermissionManager(const bool jwtEnabled)
        : _jwtEnabled(jwtEnabled) {}

    // PUBLIC_INTERFACE
    bool IsAllowed(const string& appId, const string& requiredGroup) const {
        // Phase 1: allow all. Extend in Phase 2 using JWT and permission groups.
        (void)appId;
        (void)requiredGroup;
        return true;
    }

    bool JwtEnabled() const { return _jwtEnabled; }

private:
    bool _jwtEnabled { false };
};

} // namespace Plugin
} // namespace WPEFramework
