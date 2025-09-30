#include "App2AppProvider.h"

#include <plugins/IShell.h>
#include <core/JSON.h>
#include <tracing/tracing.h>
#include <cstdio>

namespace WPEFramework {
namespace Plugin {

static constexpr const TCHAR* kContext     = _T("context");
static constexpr const TCHAR* kRegister    = _T("register");
static constexpr const TCHAR* kCapability  = _T("capability");
static constexpr const TCHAR* kPayload     = _T("payload");
static constexpr const TCHAR* kCorrelation = _T("correlationId");
static constexpr const TCHAR* kResult      = _T("result");
static constexpr const TCHAR* kError       = _T("error");

static constexpr const TCHAR* kRequestId   = _T("requestId");
static constexpr const TCHAR* kConnectionId= _T("connectionId");
static constexpr const TCHAR* kAppId       = _T("appId");

SERVICE_REGISTRATION(App2AppProvider, APP2APPPROVIDER_VERSION_MAJOR, APP2APPPROVIDER_VERSION_MINOR, APP2APPPROVIDER_VERSION_PATCH)

App2AppProvider::App2AppProvider()
    : PluginHost::JSONRPC()
    , _service(nullptr)
    , _providers(new ProviderRegistry())
    , _correlations(new CorrelationMap())
    , _gateway(new GatewayClient())
    , _perms(nullptr)
    , _running(false)
    , _gatewayCallsign(_T("AppGateway"))
    , _jwtEnabled(false)
    , _policy(_T("lastWins")) {

    // Debug print (consider replacing with Thunder tracing macros)
    printf("Hello from App2AppProvider constructor2!\n");

    // Register version info for documentation
    RegisterVersion(_T("App2AppProvider"), APP2APPPROVIDER_VERSION_MAJOR, APP2APPPROVIDER_VERSION_MINOR, APP2APPPROVIDER_VERSION_PATCH);

    // Register JSON-RPC methods
    Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer>(_T("registerProvider"),
        &App2AppProvider::registerProvider, this);
    Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer>(_T("invokeProvider"),
        &App2AppProvider::invokeProvider, this);
    Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer>(_T("handleProviderResponse"),
        &App2AppProvider::handleProviderResponse, this);
    Register<Core::JSON::VariantContainer, Core::JSON::VariantContainer>(_T("handleProviderError"),
        &App2AppProvider::handleProviderError, this);
}

App2AppProvider::~App2AppProvider() {
    Unregister(_T("registerProvider"));
    Unregister(_T("invokeProvider"));
    Unregister(_T("handleProviderResponse"));
    Unregister(_T("handleProviderError"));
}

const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    _service = service;

    // Load configuration
    string configLine = service->ConfigLine();
    Config config;
    if (!configLine.empty()) {
        config.FromString(configLine);
    }

    _gatewayCallsign = config.gatewayCallsign.Value().empty() ? _T("AppGateway") : config.gatewayCallsign.Value();
    _jwtEnabled = config.jwtEnabled.Value();
    _policy = config.providerConflictPolicy.Value().empty() ? _T("lastWins") : config.providerConflictPolicy.Value();

    // Acquire token from SecurityAgent if needed (Phase 2). For now, leave empty.
    string token;

    // Initialize components
    _gateway->Initialize(service, _gatewayCallsign, token);
    _perms.reset(new PermissionManager(_jwtEnabled));

    _running = true;
    return string();
}

void App2AppProvider::Deinitialize(PluginHost::IShell* /*service*/) {
    _running = false;
    if (_correlations) {
        _correlations->Clear();
    }
    _perms.reset();
    _gateway.reset();
    _providers.reset();
    _service = nullptr;
}

string App2AppProvider::Information() const {
    return string(_T("App2AppProvider: Registers application providers and brokers consumer-provider responses via AppGateway.respond.")); 
}

bool App2AppProvider::ParseContext(const Core::JSON::VariantContainer& obj, ConsumerContext& outCtx, string& error) const {
    error.clear();
    if (!obj.HasLabel(kRequestId) || !obj.HasLabel(kConnectionId) || !obj.HasLabel(kAppId)) {
        error = _T("INVALID_CONTEXT");
        return false;
    }
    // Extract required fields
    outCtx.requestId = static_cast<uint32_t>(obj[kRequestId].Number());
    outCtx.connectionId = obj[kConnectionId].String();
    outCtx.appId = obj[kAppId].String();
    if (outCtx.connectionId.empty() || outCtx.appId.empty()) {
        error = _T("INVALID_CONTEXT");
        return false;
    }
    return true;
}

bool App2AppProvider::ParseProviderContext(const Core::JSON::VariantContainer& obj, ProviderContext& outCtx, string& error) const {
    error.clear();
    if (!obj.HasLabel(kConnectionId) || !obj.HasLabel(kAppId)) {
        error = _T("INVALID_CONTEXT");
        return false;
    }
    outCtx.connectionId = obj[kConnectionId].String();
    outCtx.appId = obj[kAppId].String();
    if (outCtx.connectionId.empty() || outCtx.appId.empty()) {
        error = _T("INVALID_CONTEXT");
        return false;
    }
    return true;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::registerProvider(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& /*response*/) {
    // Expected params:
    // { "context": {requestId, connectionId, appId}, "register": boolean, "capability": string }
    if (!params.HasLabel(kContext) || !params.HasLabel(kRegister) || !params.HasLabel(kCapability)) {
        return Core::ERROR_BAD_REQUEST;
    }

    ProviderContext providerCtx;
    string parseError;
    if (!ParseProviderContext(params[kContext].Object(), providerCtx, parseError)) {
        return Core::ERROR_BAD_REQUEST;
    }

    bool doRegister = params[kRegister].Boolean();
    string capability = params[kCapability].String();

    if (capability.empty()) {
        return Core::ERROR_BAD_REQUEST;
    }

    string regError;
    if (doRegister) {
        const bool lastWins = (_policy == _T("lastWins"));
        bool ok = _providers->Register(capability, providerCtx, lastWins, regError);
        return ok ? Core::ERROR_NONE : Core::ERROR_DUPLICATE_KEY;
    } else {
        bool ok = _providers->Unregister(capability, providerCtx.appId, regError);
        return ok ? Core::ERROR_NONE : Core::ERROR_UNKNOWN_KEY;
    }
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::invokeProvider(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response) {
    // Expected params:
    // { "context": {requestId, connectionId, appId}, "capability": string, "payload"?: object }
    if (!params.HasLabel(kContext) || !params.HasLabel(kCapability)) {
        return Core::ERROR_BAD_REQUEST;
    }

    ConsumerContext consumerCtx;
    string parseError;
    if (!ParseContext(params[kContext].Object(), consumerCtx, parseError)) {
        return Core::ERROR_BAD_REQUEST;
    }

    string capability = params[kCapability].String();
    if (capability.empty()) {
        return Core::ERROR_BAD_REQUEST;
    }

    ProviderContext provider;
    if (!_providers->Resolve(capability, provider)) {
        // Provider not found
        return Core::ERROR_UNKNOWN_KEY;
    }

    // Track correlation for provider's upcoming response
    const string correlationId = _correlations->Create(consumerCtx);

    // Return the correlationId to caller
    Core::JSON::VariantContainer resultObject;
    Core::JSON::Variant cid(correlationId.c_str());
    resultObject.Set(kCorrelation, cid);

    Core::JSON::Variant vResult(resultObject);
    response.Set(_T("result"), vResult);

    // By design, provider invocation is triggered externally (e.g. via AppGateway resolver patterns).
    // App2AppProvider responsibility here is to create correlation and return correlationId.
    return Core::ERROR_NONE;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::handleProviderResponse(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& /*response*/) {
    // Expected params: { "payload": { "correlationId": string, "result": object }, "capability": string }
    if (!params.HasLabel(kPayload) || !params.HasLabel(kCapability)) {
        return Core::ERROR_BAD_REQUEST;
    }
    Core::JSON::VariantContainer payload = params[kPayload].Object();
    if (!payload.HasLabel(kCorrelation) || !payload.HasLabel(kResult)) {
        return Core::ERROR_BAD_REQUEST;
    }

    const string correlationId = payload[kCorrelation].String();

    ConsumerContext consumerCtx;
    if (!_correlations->Take(correlationId, consumerCtx)) {
        // Unknown correlation
        return Core::ERROR_UNKNOWN_KEY;
    }

    // Build payload to forward via AppGateway.respond: wrap incoming 'result' into payload
    Core::JSON::VariantContainer forwardPayload;
    Core::JSON::Variant vResult(payload[kResult].Object());
    forwardPayload.Set(kResult, vResult);

    const uint32_t rc = _gateway->Respond(consumerCtx, forwardPayload);
    return rc;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::handleProviderError(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& /*response*/) {
    // Expected params: { "payload": { "correlationId": string, "error": { code:number, message:string } }, "capability": string }
    if (!params.HasLabel(kPayload) || !params.HasLabel(kCapability)) {
        return Core::ERROR_BAD_REQUEST;
    }
    Core::JSON::VariantContainer payload = params[kPayload].Object();
    if (!payload.HasLabel(kCorrelation) || !payload.HasLabel(kError)) {
        return Core::ERROR_BAD_REQUEST;
    }

    const string correlationId = payload[kCorrelation].String();

    ConsumerContext consumerCtx;
    if (!_correlations->Take(correlationId, consumerCtx)) {
        // Unknown correlation
        return Core::ERROR_UNKNOWN_KEY;
    }

    // Build payload to forward via AppGateway.respond: wrap incoming 'error' into payload
    Core::JSON::VariantContainer forwardPayload;
    Core::JSON::Variant vError(payload[kError].Object());
    forwardPayload.Set(kError, vError);

    const uint32_t rc = _gateway->Respond(consumerCtx, forwardPayload);
    return rc;
}

} // namespace Plugin
} // namespace WPEFramework
