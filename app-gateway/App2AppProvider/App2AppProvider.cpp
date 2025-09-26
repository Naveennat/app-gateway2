#include "App2AppProvider.h"

namespace WPEFramework {
namespace Plugin {

namespace {
    // Common JSON-RPC error codes used in the design documents.
    constexpr uint32_t JSONRPC_PARSE_ERROR      = static_cast<uint32_t>(-32700);
    constexpr uint32_t JSONRPC_INVALID_PARAMS   = static_cast<uint32_t>(-32602);
    constexpr uint32_t JSONRPC_INVALID_REQUEST  = static_cast<uint32_t>(-32699);
}

App2AppProvider::App2AppProvider()
    : PluginHost::JSONRPC()
    , _service(nullptr)
    , _providers(new ProviderRegistry())
    , _correlations(new CorrelationMap())
    , _gateway()
    , _perms(new PermissionManager())
    , _gatewayCallsign("org.rdk.AppGateway")
    , _jwtEnabled(false)
    , _conflictPolicy("lastWins")
    , _securityToken() {
    RegisterMethods();
}

App2AppProvider::~App2AppProvider() {
    UnregisterMethods();
}

void App2AppProvider::RegisterMethods() {
    Register<RegisterProviderParams, Core::JSON::Container>(_T("registerProvider"), &App2AppProvider::endpoint_registerProvider, this);
    Register<InvokeProviderParams, InvokeProviderResult>(_T("invokeProvider"), &App2AppProvider::endpoint_invokeProvider, this);
    Register<ProviderResponseParams, Core::JSON::Container>(_T("handleProviderResponse"), &App2AppProvider::endpoint_handleProviderResponse, this);
    Register<ProviderResponseParams, Core::JSON::Container>(_T("handleProviderError"), &App2AppProvider::endpoint_handleProviderError, this);
}

void App2AppProvider::UnregisterMethods() {
    Unregister(_T("registerProvider"));
    Unregister(_T("invokeProvider"));
    Unregister(_T("handleProviderResponse"));
    Unregister(_T("handleProviderError"));
}

bool App2AppProvider::ExtractSecurityToken(PluginHost::IShell* service, string& token) const {
    // Acquire a token from the SecurityAgent using the current API (CreateToken).
    bool rc = false;
    token.clear();

    if (service != nullptr) {
        PluginHost::IAuthenticate* authenticate =
            service->QueryInterfaceByCallsign<PluginHost::IAuthenticate>(_T("SecurityAgent"));
        if (authenticate != nullptr) {
            string generated;
            if (authenticate->CreateToken(0 /* length */, nullptr /* buffer */, generated) == Core::ERROR_NONE) {
                token = generated;
                rc = true;
            }
            authenticate->Release();
        }
    }
    return rc;
}

bool App2AppProvider::ValidateContext(const Context& ctx) const {
    return (ctx.RequestId.IsSet() && ctx.ConnectionId.IsSet() && ctx.AppId.IsSet()
        && !ctx.ConnectionId.Value().empty() && !ctx.AppId.Value().empty());
}

const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    ASSERT(service != nullptr);
    _service = service;

    // Load configuration
    Config config;
    config.FromString(service->ConfigLine());

    _jwtEnabled = config.JwtEnabled.Value();
    _gatewayCallsign = (config.GatewayCallsign.IsSet() ? config.GatewayCallsign.Value() : string("org.rdk.AppGateway"));
    _conflictPolicy = (config.ProviderConflictPolicy.IsSet() ? config.ProviderConflictPolicy.Value() : string("lastWins"));

    _perms->JwtEnabled(_jwtEnabled);
    _providers->SetLastWinsPolicy(_conflictPolicy == "lastWins");

    // Gather security token for local JSON-RPC usage to AppGateway.respond
    ExtractSecurityToken(service, _securityToken);
    _gateway.reset(new GatewayClient(service, _gatewayCallsign, _securityToken));

    return string();
}

void App2AppProvider::Deinitialize(PluginHost::IShell* service) {
    _gateway.reset();
    _providers->Clear();
    _correlations->Clear();
    _perms.reset();
    _providers.reset();
    _correlations.reset();

    _securityToken.clear();
    _service = nullptr;

    (void)service;
}

string App2AppProvider::Information() const {
    // Return empty string (typical placeholder)
    return string();
}

// JSON-RPC: registerProvider
uint32_t App2AppProvider::endpoint_registerProvider(const RegisterProviderParams& params, Core::JSON::Container& /*response*/) {
    if (!ValidateContext(params.Ctx) || !params.Register.IsSet() || !params.Capability.IsSet() || params.Capability.Value().empty()) {
        return JSONRPC_INVALID_PARAMS;
    }

    if (!_perms->IsAllowed(params.Ctx.AppId.Value(), string())) {
        return Core::ERROR_GENERAL;
    }

    const bool reg = params.Register.Value();
    const string capability = params.Capability.Value();

    if (reg) {
        ProviderRegistry::ProviderContext pctx;
        pctx.appId = params.Ctx.AppId.Value();
        pctx.connectionId = params.Ctx.ConnectionId.Value();
        const bool ok = _providers->Register(capability, pctx);
        if (!ok) {
            return JSONRPC_INVALID_REQUEST; // duplicate and policy=rejectDuplicates
        }
    } else {
        const bool ok = _providers->Unregister(capability, params.Ctx.AppId.Value());
        if (!ok) {
            return JSONRPC_INVALID_REQUEST; // not found or not owner
        }
    }

    return Core::ERROR_NONE;
}

// JSON-RPC: invokeProvider
uint32_t App2AppProvider::endpoint_invokeProvider(const InvokeProviderParams& params, InvokeProviderResult& response) {
    if (!ValidateContext(params.Ctx) || !params.Capability.IsSet() || params.Capability.Value().empty()) {
        return JSONRPC_INVALID_PARAMS;
    }

    const string capability = params.Capability.Value();

    ProviderRegistry::ProviderContext pctx;
    if (!_providers->Resolve(capability, pctx)) {
        // Provider not found for capability
        return JSONRPC_INVALID_REQUEST;
    }

    // Create correlation for consumer
    CorrelationMap::ConsumerContext cc;
    cc.appId = params.Ctx.AppId.Value();
    cc.connectionId = params.Ctx.ConnectionId.Value();
    cc.requestId = params.Ctx.RequestId.Value();

    const string correlationId = _correlations->Create(cc);
    response.CorrelationId = correlationId;

    // Phase 1 does not actively forward an invocation to provider (this depends on AppGateway transport patterns).
    // The provider, upon receiving the request through its own channel, will reply using handleProviderResponse/Error.

    return Core::ERROR_NONE;
}

// JSON-RPC: handleProviderResponse
uint32_t App2AppProvider::endpoint_handleProviderResponse(const ProviderResponseParams& params, Core::JSON::Container& /*response*/) {
    if (!params.Payload.IsSet() || !params.Capability.IsSet() || params.Capability.Value().empty()) {
        return JSONRPC_INVALID_PARAMS;
    }

    // Extract correlationId
    if (!params.Payload.HasLabel(_T("correlationId"))) {
        return JSONRPC_INVALID_PARAMS;
    }
    const string correlationId = params.Payload.Get(_T("correlationId")).String();

    CorrelationMap::ConsumerContext cc;
    if (!_correlations->Take(correlationId, cc)) {
        return JSONRPC_INVALID_REQUEST; // unknown correlation
    }

    // Extract result object (if any)
    Core::JSON::Object payloadToApp;
    if (params.Payload.HasLabel(_T("result"))) {
        // Assign nested object explicitly using Object() to avoid treating it as a number.
        payloadToApp[_T("result")].Object(params.Payload.Get(_T("result")).Object());
    } else {
        // No result supplied is a parse/params issue
        return JSONRPC_INVALID_PARAMS;
    }

    // Build context object for AppGateway.respond
    Core::JSON::Object ctx;
    // Assign primitive types to JSON::Variant to avoid template mismatch on wrapper types.
    ctx[_T("requestId")] = static_cast<uint32_t>(cc.requestId);
    ctx[_T("connectionId")] = cc.connectionId;
    ctx[_T("appId")] = cc.appId;

    // Use GatewayClient to forward
    const uint32_t rc = _gateway->Respond(ctx, payloadToApp);
    return rc;
}

// JSON-RPC: handleProviderError
uint32_t App2AppProvider::endpoint_handleProviderError(const ProviderResponseParams& params, Core::JSON::Container& /*response*/) {
    if (!params.Payload.IsSet() || !params.Capability.IsSet() || params.Capability.Value().empty()) {
        return JSONRPC_INVALID_PARAMS;
    }

    // Extract correlationId
    if (!params.Payload.HasLabel(_T("correlationId"))) {
        return JSONRPC_INVALID_PARAMS;
    }
    const string correlationId = params.Payload.Get(_T("correlationId")).String();

    CorrelationMap::ConsumerContext cc;
    if (!_correlations->Take(correlationId, cc)) {
        return JSONRPC_INVALID_REQUEST; // unknown correlation
    }

    // Extract error object (if any)
    Core::JSON::Object payloadToApp;
    if (params.Payload.HasLabel(_T("error"))) {
        // Assign nested object explicitly using Object() to avoid treating it as a number.
        payloadToApp[_T("error")].Object(params.Payload.Get(_T("error")).Object());
    } else {
        // No error supplied is a parse/params issue
        return JSONRPC_INVALID_PARAMS;
    }

    // Build context object for AppGateway.respond
    Core::JSON::Object ctx;
    ctx[_T("requestId")] = Core::JSON::DecUInt32(cc.requestId);
    ctx[_T("connectionId")] = Core::JSON::String(cc.connectionId);
    ctx[_T("appId")] = Core::JSON::String(cc.appId);

    // Use GatewayClient to forward
    const uint32_t rc = _gateway->Respond(ctx, payloadToApp);
    return rc;
}

} // namespace Plugin
} // namespace WPEFramework
