#pragma once
/*
 * Minimal stub for "UtilsLogging.h" used by the AppGateway plugin in this
 * repository's isolated build.
 *
 * The upstream implementation lives in external helper repos; for L0 build
 * verification we only require compilation. Provide no-op logging helpers.
 */
#include <string>

namespace UtilsLogging {
inline void Info(const std::string&) {}
inline void Warn(const std::string&) {}
inline void Error(const std::string&) {}
} // namespace UtilsLogging
