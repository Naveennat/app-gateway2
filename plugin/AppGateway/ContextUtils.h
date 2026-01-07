#pragma once
#include <cstdint>
#include <string>

#include "../../interfaces/IAppGateway.h"

namespace ContextUtils {

inline uint32_t RequestId(const WPEFramework::Exchange::GatewayContext& ctx) { return ctx.requestId; }
inline uint32_t ConnectionId(const WPEFramework::Exchange::GatewayContext& ctx) { return ctx.connectionId; }
inline const std::string& AppId(const WPEFramework::Exchange::GatewayContext& ctx) { return ctx.appId; }

// Minimal origin check used by some code paths.
inline bool IsOriginGateway(const std::string& origin) { return !origin.empty(); }

} // namespace ContextUtils
