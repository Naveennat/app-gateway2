#include "AppGateway.h"
#include <cstdio>

namespace WPEFramework {
namespace Plugin {

namespace {
    // Common JSON-RPC error codes used in the design documents.
    constexpr uint32_t JSONRPC_INVALID_PATH = 2;        // configure
    constexpr uint32_t JSONRPC_PARSE_ERROR = static_cast<uint32_t>(-32700);
    constexpr uint32_t JSONRPC_INVALID_PARAMS = static_cast<uint32_t>(-32602);
    constexpr uint32_t JSONRPC_INVALID_REQUEST = static_cast<uint32_t>(-32699);
}

AppGateway::AppGateway()
    : PluginHost::JSONRPC()
    , _service(nullptr)
    , _resolver(new Resolver())
    , _router()
    , _connections(new ConnectionRegistry())
    , _perms(new PermissionManager())
    , _ws(new GatewayWebSocket())
    , _securityToken()
    , _wsPort(3473)
    , _permissionEnforcement(true)
    , _jwtEnabled(false)
    , _callsign("org.rdk.AppGateway") {
    std::printf("Hello from AppGateway constructor8!\n");
    RegisterMethods();
}

AppGateway::~AppGateway() {
    UnregisterMethods();
}

void AppGateway::RegisterMethods() {
    Register<ConfigureParams, Core::JSON::Container>(_T("configure"), &AppGateway::endpoint_configure, this);
    Register<RespondParams, Core::JSON::Container>(_T("respond"), &AppGateway::endpoint_respond, this);
    Register<ResolveParams, ResolveResult>(_T("resolve"), &AppGateway::endpoint_resolve, this);
}

void AppGateway::UnregisterMethods() {
    Unregister(_T("configure"));
    Unregister(_T("respond"));
    Unregister(_T("resolve"));
}

bool AppGateway::ExtractSecurityToken(PluginHost::IShell* service, string& token) const {
    bool rc = false;
    if (service != nullptr) {
        auto* authenticate = service->QueryInterfaceByCallsign<PluginHost::IAuthenticate>(_T("SecurityAgent"));
        if (authenticate != nullptr) {
            string payload;
            uint8_t buffer[Token::MAX_TOKEN_SIZE];
            const uint16_t length = authenticate->Token(lengthof(buffer), buffer);
            if (length > 0) {
                token = string(reinterpret_cast<const char*>(buffer), length);
                rc = true;
            }
            authenticate->Release();
        }
    }
    return rc;
}

const string AppGateway::Initialize(PluginHost::IShell* service) {
    ASSERT(service != nullptr);
    _service = service;

    // Load configuration
    Config config;
    config.FromString(service->ConfigLine());
    _wsPort = config.ServerPort.Value();
    _permissionEnforcement = config.PermissionEnforcement.Value();
    _jwtEnabled = config.JwtEnabled.Value();
    _callsign = (config.GatewayCallsign.IsSet() ? config.GatewayCallsign.Value() : string("org.rdk.AppGateway"));
    _perms->JwtEnabled(_jwtEnabled);

    // Gather security token for local JSON-RPC usage
    ExtractSecurityToken(service, _securityToken);

    // Router uses token for local dispatch
    _router.reset(new RequestRouter(service, _securityToken));

    // Start WS if configured
    if (_wsPort > 0) {
        string err;
        if (!_ws->Start(_wsPort, err)) {
            SYSLOG(Logging::Startup, (_T("AppGateway: WS start failed on port %u: %s"), _wsPort, err.c_str()));
        }
    }

    // Load resolution overlays if provided
    if (config.ResolutionPaths.Length() > 0) {
        std::vector<string> paths;
        Core::JSON::ArrayType<Core::JSON::String>::Iterator it(config.ResolutionPaths.Elements());
        while (it.Next()) {
            paths.emplace_back(it.Current().Value());
        }
        string err;
        if (!_resolver->LoadPaths(paths, err)) {
            SYSLOG(Logging::Startup, (_T("AppGateway: resolution load failed: %s"), err.c_str()));
        }
    }

    return string();
}

void AppGateway::Deinitialize(PluginHost::IShell* service) {
    if (_ws && _ws->Running()) {
        _ws->Stop();
    }

    _router.reset();
    _resolver.reset();
    _connections.reset();
    _perms.reset();
    _ws.reset();

    _securityToken.clear();
    _service = nullptr;
    (void)service;
}

string AppGateway::Information() const {
    // Return empty string (typical placeholder)
    return string();
}

// JSON-RPC: configure
uint32_t AppGateway::endpoint_configure(const ConfigureParams& params, Core::JSON::Container& /*response*/) {
    if (params.Paths.Length() == 0) {
        return JSONRPC_INVALID_PARAMS;
    }

    std::vector<string> paths;
    {
        Core::JSON::ArrayType<Core::JSON::String>::ConstIterator it(params.Paths.Elements());
        while (it.Next()) {
            if (it.Current().Value().empty() == false) {
                paths.emplace_back(it.Current().Value());
            }
        }
    }

    if (paths.empty()) {
        return JSONRPC_INVALID_PARAMS;
    }

    string err;
    if (!_resolver->LoadPaths(paths, err)) {
        SYSLOG(Logging::Notification, (_T("AppGateway.configure: invalid path(s): %s"), err.c_str()));
        return JSONRPC_INVALID_PATH;
    }

    return Core::ERROR_NONE;
}

// JSON-RPC: respond
uint32_t AppGateway::endpoint_respond(const RespondParams& params, Core::JSON::Container& /*response*/) {
    // Validate context
    if (!params.Ctx.RequestId.IsSet() || !params.Ctx.ConnectionId.IsSet() || !params.Ctx.AppId.IsSet()) {
        return JSONRPC_INVALID_PARAMS;
    }

    // Determine payload
    Core::JSON::Object effectivePayload;
    if (params.Payload.IsEmpty() == false) {
        effectivePayload = params.Payload;
    } else if (params.Result.IsEmpty() == false || params.Error.IsEmpty() == false) {
        // Wrap legacy fields into payload
        if (params.Result.IsEmpty() == false) {
            effectivePayload.Set(_T("result"), params.Result);
        }
        if (params.Error.IsEmpty() == false) {
            effectivePayload.Set(_T("error"), params.Error);
        }
    } else {
        return JSONRPC_INVALID_PARAMS;
    }

    // For Phase 1, this is a no-op stub (integration with a WS endpoint is product-specific).
    // If a transport is present, deliver the payload; otherwise, accept silently.
    if (_ws && _ws->Running()) {
        Core::JSON::Object envelope;
        envelope.Set(_T("context"), params.Ctx);
        envelope.Set(_T("payload"), effectivePayload);

        string serialized;
        envelope.ToString(serialized);
        _ws->SendTo(params.Ctx.ConnectionId.Value(), serialized);
    }

    return Core::ERROR_NONE;
}

// JSON-RPC: resolve
uint32_t AppGateway::endpoint_resolve(const ResolveParams& params, ResolveResult& response) {
    if (!params.Method.IsSet() || params.Method.Value().empty()) {
        return JSONRPC_INVALID_PARAMS;
    }

    Core::JSON::Object reso;
    string appId;
    if (params.Ctx.HasLabel(_T("appId"))) {
        appId = params.Ctx.Get<Core::JSON::String>(_T("appId")).Value();
    }

    // Try resolver first
    bool found = _resolver->Get(appId, params.Method.Value(), params.Params, reso);

    // Fallback default echo if not found
    if (!found) {
        // default resolution is echo alias
        reso.Set(_T("alias"), Core::JSON::String(params.Method.Value()));
    }

    response.Resolution = reso;
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
