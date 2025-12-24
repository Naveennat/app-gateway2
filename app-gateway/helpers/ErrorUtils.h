#pragma once

// PUBLIC_INTERFACE
// Wrapper header for ErrorUtils utilities used across this repo.
//
// The canonical implementation in this repo lives in Supporting_Files/UtilsFirebolt.h
// and defines a global `class ErrorUtils` with static helper methods.
// Some AppGateway code also expects `AppGwErrorUtils::NotPermitted(...)`, so we
// provide that alias here.
//
// NOTE: Do NOT define `namespace ErrorUtils` here (it conflicts with the class).

#include <UtilsFirebolt.h>

// UtilsFirebolt.h defines generic macro names that collide with Thunder Core
// constants (e.g., WPEFramework::Core::ERROR_NOT_SUPPORTED). Ensure they don't
// leak beyond the Firebolt helper implementation.
#ifdef ERROR_NOT_SUPPORTED
#undef ERROR_NOT_SUPPORTED
#endif
#ifdef ERROR_NOT_AVAILABLE
#undef ERROR_NOT_AVAILABLE
#endif
#ifdef ERROR_NOT_PERMITTED
#undef ERROR_NOT_PERMITTED
#endif

namespace AppGwErrorUtils {

    // PUBLIC_INTERFACE
    static inline void NotPermitted(string& resolutionOut)
    {
        /** AppGateway-specific helper used by AppGatewayImplementation.cpp. */
        resolutionOut = ErrorUtils::GetFireboltError(FireboltError::NOT_PERMITTED);
    }

} // namespace AppGwErrorUtils
