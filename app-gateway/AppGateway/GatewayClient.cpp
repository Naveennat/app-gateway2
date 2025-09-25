#include "GatewayClient.h"

namespace WPEFramework {
namespace Plugin {

uint32_t GatewayClient::Respond(const Core::JSON::Object& context, const Core::JSON::Object& payload) {
    // A real implementation would JSONRPC-invoke org.rdk.AppGateway.1.respond using a local dispatcher.
    (void)_service;
    (void)_gatewayCallsign;
    (void)_securityToken;
    (void)context;
    (void)payload;
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
