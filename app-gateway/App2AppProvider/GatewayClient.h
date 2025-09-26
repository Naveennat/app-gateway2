#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * GatewayClient
 * Encapsulates local JSON-RPC call to AppGateway.respond.
 * Phase 1: Stubbed to return success.
 */
class GatewayClient {
public:
    GatewayClient(PluginHost::IShell* service, const string& callsign, const string& token)
        : _service(service)
        , _gatewayCallsign(callsign)
        , _securityToken(token) {
    }

    // PUBLIC_INTERFACE
    /**
     * Respond to an application context via AppGateway.respond.
     * Returns Core::ERROR_... codes.
     */
    uint32_t Respond(const Core::JSON::Object& context, const Core::JSON::Object& payload);

private:
    PluginHost::IShell* _service;
    string _gatewayCallsign;
    string _securityToken;
};

} // namespace Plugin
} // namespace WPEFramework
