#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * GatewayClient
 * Allows calling AppGateway.respond locally (used by other plugins).
 * Here, this is a stub for integration expansion.
 */
class GatewayClient {
public:
    GatewayClient(PluginHost::IShell* service, const string& callsign, const string& token)
        : _service(service)
        , _gatewayCallsign(callsign)
        , _securityToken(token) {
    }

    // PUBLIC_INTERFACE
    uint32_t Respond(const Core::JSON::Object& context, const Core::JSON::Object& payload);

private:
    PluginHost::IShell* _service;
    string _gatewayCallsign;
    string _securityToken;
};

} // namespace Plugin
} // namespace WPEFramework
