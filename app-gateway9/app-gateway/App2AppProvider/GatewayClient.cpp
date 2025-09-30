#include "Module.h"
#include "GatewayClient.h"

namespace WPEFramework {
namespace Plugin {

uint32_t GatewayClient::Respond(const ConsumerContext& ctx, const Core::JSON::VariantContainer& payload) const {
    // Build params: { "context": {requestId, connectionId, appId}, "payload": { ... } }
    Core::JSON::VariantContainer params;
    Core::JSON::VariantContainer jsonContext;

    Core::JSON::Variant requestId(static_cast<int64_t>(ctx.requestId));
    Core::JSON::Variant connectionId(ctx.connectionId.c_str());
    Core::JSON::Variant appId(ctx.appId.c_str());

    jsonContext.Set(_T("requestId"), requestId);
    jsonContext.Set(_T("connectionId"), connectionId);
    jsonContext.Set(_T("appId"), appId);

    Core::JSON::Variant vCtx(jsonContext);
    Core::JSON::Variant vPayload(payload);

    params.Set(_T("context"), vCtx);
    params.Set(_T("payload"), vPayload);

    // Use SmartLinkType to communicate with the gateway plugin.
    WPEFramework::JSONRPC::SmartLinkType<Core::JSON::IElement> link(_gatewayCallsign, _T("A2AP_GatewayClient"));

    // Use overload that takes VariantContainer directly (uses default wait time internally)
    Core::JSON::VariantContainer response;
    uint32_t result = link.Invoke(_T("respond"), params, response);

    return result;
}

} // namespace Plugin
} // namespace WPEFramework
