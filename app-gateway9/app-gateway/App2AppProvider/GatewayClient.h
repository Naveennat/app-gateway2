#pragma once

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IShell.h>
#include <websocket/JSONRPCLink.h>

#include "CorrelationMap.h"

namespace WPEFramework {
namespace Plugin {

// GatewayClient encapsulates JSON-RPC call to the AppGateway.respond method.
class GatewayClient {
public:
    GatewayClient()
        : _service(nullptr), _gatewayCallsign(_T("AppGateway")), _token() {}

    void Initialize(PluginHost::IShell* service, const string& callsign, const string& token) {
        _service = service;
        _gatewayCallsign = callsign.empty() ? _T("AppGateway") : callsign;
        _token = token;
    }

    // Respond to the consumer via AppGateway.respond.
    // Returns Core::ERROR_NONE on success, or a framework error code.
    uint32_t Respond(const ConsumerContext& ctx, const Core::JSON::VariantContainer& payload) const;

private:
    PluginHost::IShell* _service;
    string _gatewayCallsign;
    string _token;
};

} // namespace Plugin
} // namespace WPEFramework
