#pragma once

// PUBLIC_INTERFACE
// Minimal ErrorUtils stub providing NotPermitted helper used by AppGatewayImplementation.cpp.
#include <string>

namespace AppGwErrorUtils {

// PUBLIC_INTERFACE
static inline void NotPermitted(const std::string& /*resolution*/) {
    // No-op stub. Real implementation would log or track permission errors.
}

 // PUBLIC_INTERFACE
static inline void CustomBadRequest(const std::string& /*message*/, std::string& /*resolutionOut*/) {
    // No-op stub. Real implementation would set error message in resolution JSON.
}

// PUBLIC_INTERFACE
static inline void CustomInternal(const std::string& /*message*/, std::string& /*resolutionOut*/) {
    // No-op stub for internal error mapping.
}

// PUBLIC_INTERFACE
static inline void NotAvailable(std::string& /*resolutionOut*/) {
    // No-op stub for not available mapping.
}

// PUBLIC_INTERFACE
static inline void CustomInitialize(const std::string& /*message*/, std::string& /*resolutionOut*/) {
    // No-op stub for initialize failure mapping.
}

// PUBLIC_INTERFACE
static inline void NotSupported(std::string& /*resolutionOut*/) {
    // No-op stub for not supported mapping.
}

} // namespace ErrorUtils
