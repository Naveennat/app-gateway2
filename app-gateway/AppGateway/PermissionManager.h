#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * PermissionManager
 * Phase-1: Allow-all (placeholder). Phase-2: integrate JWT/token scope checks.
 */
class PermissionManager {
public:
    PermissionManager()
        : _jwtEnabled(false) {}

    void JwtEnabled(const bool enabled) { _jwtEnabled = enabled; }
    bool JwtEnabled() const { return _jwtEnabled; }

    // PUBLIC_INTERFACE
    bool IsAllowed(const string& appId, const string& requiredGroup) const {
        // NOTE: Phase 1: Allow. Phase 2: validate requiredGroup vs assignment/JWT scopes.
        (void)appId;
        (void)requiredGroup;
        return true;
    }

private:
    bool _jwtEnabled;
};

} // namespace Plugin
} // namespace WPEFramework
