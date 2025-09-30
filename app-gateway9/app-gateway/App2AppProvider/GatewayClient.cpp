#include "GatewayClient.h"

namespace WPEFramework {
namespace Plugin {

uint32_t GatewayClient::Respond(const ConsumerContext& ctx, const Core::JSON::Object& payload) const {
    // Build params: { "context": {requestId, connectionId, appId}, "payload": { ... } }
    Core::JSON::Object params;
    Core::JSON::Object jsonContext;

    Core::JSON::DecUInt32 requestId;
    requestId = ctx.requestId;
    Core::JSON::String connectionId;
    connectionId = ctx.connectionId;
    Core::JSON::String appId;
    appId = ctx.appId;

    jsonContext.Set(_T("requestId"), requestId);
    jsonContext.Set(_T("connectionId"), connectionId);
    jsonContext.Set(_T("appId"), appId);

    params.Set(_T("context"), jsonContext);
    params.Set(_T("payload"), payload);

    // Serialize
    string serialized;
    params.ToString(serialized);

    // Use SmartLinkType to communicate with the controller to the gateway plugin.
    // Pass the callsign, and let the link append the version ".1" if included in callsign string.
    WPEFramework::JSONRPC::SmartLinkType<Core::JSON::IElement> link(_gatewayCallsign, _T("A2AP_GatewayClient"));

    Core::ProxyType<Core::JSONRPC::Message> response;
    // Invoke method "respond" with serialized string
    uint32_t result = link.Invoke(WPEFramework::JSONRPC::SmartLinkType<Core::JSON::IElement>::Connection::DefaultWaitTime,
                                  _T("respond"),
                                  serialized,
                                  response);
    if (result != Core::ERROR_NONE) {
        return result;
    }
    if (response.IsValid() == true && response->Error.IsSet() == true) {
        // Return framework error code
        return static_cast<uint32_t>(response->Error.Code.Value());
    }
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
