#pragma once
#include <string>

namespace ErrorUtils {
// Minimal stubs for isolated compilation; real implementation populates JSON-RPC error payloads.
inline void NotAvailable(std::string& /*out*/) {}
inline void NotSupported(std::string& /*out*/) {}
inline void NotPermitted(std::string& /*out*/) {}

inline void CustomInternal(const char* /*msg*/, std::string& /*out*/) {}
inline void CustomBadRequest(const char* /*msg*/, std::string& /*out*/) {}
inline void CustomInitialize(const char* /*msg*/, std::string& /*out*/) {}
} // namespace ErrorUtils
