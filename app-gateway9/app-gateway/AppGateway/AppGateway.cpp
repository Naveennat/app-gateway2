#include "AppGateway.h"
#include <plugins/Module.h>
#include <plugins/IShell.h>
#include <plugins/IStateControl.h>
#include <core/JSON.h>
#include <tracing/Tracing.h>

namespace WPEFramework {
namespace Plugin {

// Lightweight internal component stubs (to keep this initial implementation buildable).
// These should be replaced/expanded with full implementations following the design spec.

class Resolver {
public:
    Resolver() = default;
    // Load and overlay resolution JSON files
    bool LoadPaths(const std::vector<string>& /*paths*/, string& /*error*/) {
        return true;
    }
    // Query resolution: for now, echo a trivial resolution object
    bool Get(const string& /*appId*/, const string& method, const Core::JSON::Object& /*params*/, Core::JSON::Object& out) const {
        // Minimal placeholder: alias to the same method for demonstration
        Core::JSON::String alias;
        alias = method;
        out.Set(_T("alias"), alias);
        return true;
    }
};

class RequestRouter {
public:
    RequestRouter(PluginHost::IShell* service, const string& token)
        : _service(service)
        , _securityToken(token) {}
    // Dispatch based on resolution (placeholder)
    uint32_t DispatchResolved(const Core::JSON::Object& /*resolution*/, const Core::JSON::Object& /*callParams*/, Core::JSON::Object& response) {
        // Return a trivial success for placeholder behavior
        Core::JSON::Boolean ok;
        ok = true;
        response.Set(_T("ok"), ok);
        return Core::ERROR_NONE;
    }
private:
    PluginHost::IShell* _service;
    string _securityToken;
};

class ConnectionRegistry {
public:
    ConnectionRegistry() = default;
    bool SendTo(const string& /*connectionId*/, const string& /*message*/) {
        // Placeholder: in a full implementation, this will route over GatewayWebSocket
        return true;
    }
};

class PermissionManager {
public:
    explicit PermissionManager(bool /*jwtEnabled*/) {}
    bool IsAllowed(const string& /*appId*/, const string& /*requiredGroup*/) const {
        // Placeholder: allow all for now
        return true;
    }
};

class GatewayWebSocket {
public:
    GatewayWebSocket() : _running(false), _port(0) {}
    bool Start(uint16_t port, string& /*error*/) {
        _port = port;
        _running = (port != 0);
        return _running;
    }
    void Stop() {
        _running = false;
    }
    bool SendTo(const string& /*connectionId*/, const string& /*json*/) {
        return true;
    }
private:
    std::atomic<bool> _running;
    uint16_t _port;
};

// JSON keys helpers
static constexpr const TCHAR* kContext = _T("context");
static constexpr const TCHAR* kMethod = _T("method");
static constexpr const TCHAR* kParams = _T("params");
static constexpr const TCHAR* kPaths  = _T("paths");
static constexpr const TCHAR* kPayload = _T("payload");
static constexpr const TCHAR* kResult = _T("result");
static constexpr const TCHAR* kError = _T("error");

static constexpr const TCHAR* kRequestId = _T("requestId");
static constexpr const TCHAR* kConnectionId = _T("connectionId");
static constexpr const TCHAR* kAppId = _T("appId");

SERVICE_REGISTRATION(AppGateway, APPGATEWAY_VERSION_MAJOR, APPGATEWAY_VERSION_MINOR, APPGATEWAY_VERSION_PATCH)

AppGateway::AppGateway()
    : PluginHost::JSONRPC()
    , _service(nullptr)
    , _resolver(new Resolver())
    , _router(nullptr)
    , _connections(new ConnectionRegistry())
    , _perms(nullptr)
    , _ws(new GatewayWebSocket())
    , _running(false)
    , _securityToken()
    , _serverPort(3473)
    , _permissionEnforcement(true)
    , _jwtEnabled(false) {

    // Register JSON-RPC methods: configure, resolve, respond
    Register<Core::JSON::Object, Core::JSON::Object>(_T("configure"),
        [this](const Core::JSON::Object& params, Core::JSON::Object& /*unused*/) -> uint32_t {
            Core::JSON::ArrayType<Core::JSON::String> paths;
            auto code = ParseConfigureParams(params, paths);
            if (code != Core::ERROR_NONE) {
                return code;
            }
            // Convert to std::vector<string>
            std::vector<string> p;
            Core::JSON::ArrayType<Core::JSON::String>::ConstIterator index = paths.Elements();
            while (index.Next() == true) {
                p.emplace_back(index.Current().Value());
            }
            string error;
            if (!_resolver->LoadPaths(p, error)) {
                // INVALID_PATH -> application-specific code 2
                return 2;
            }
            return Core::ERROR_NONE;
        });

    Register<Core::JSON::Object, Core::JSON::Object>(_T("resolve"),
        [this](const Core::JSON::Object& params, Core::JSON::Object& result) -> uint32_t {
            Core::JSON::Object ctx;
            Core::JSON::Object callParams;
            string method;
            const auto parse = ParseResolveParams(params, ctx, method, callParams);
            if (parse != Core::ERROR_NONE) {
                return parse;
            }
            Core::JSON::Object resolution;
            const string appId = ctx.Get(kAppId).Value();
            if (!_resolver->Get(appId, method, callParams, resolution)) {
                // For now: return empty resolution as success
            }
            // result: { "resolution": {...} }
            result.Set(_T("resolution"), resolution);
            return Core::ERROR_NONE;
        });

    Register<Core::JSON::Object, Core::JSON::Object>(_T("respond"),
        [this](const Core::JSON::Object& params, Core::JSON::Object& /*unused*/) -> uint32_t {
            Core::JSON::Object ctx;
            Core::JSON::Object payload;
            const auto parse = ParseRespondParams(params, ctx, payload);
            if (parse != Core::ERROR_NONE) {
                return parse;
            }
            const string connectionId = ctx.Get(kConnectionId).Value();
            string serialized;
            payload.ToString(serialized);
            if (!_connections->SendTo(connectionId, serialized)) {
                return Core::ERROR_GENERAL;
            }
            return Core::ERROR_NONE;
        });
}

AppGateway::~AppGateway() {
    Unregister(_T("configure"));
    Unregister(_T("resolve"));
    Unregister(_T("respond"));
}

const string AppGateway::Initialize(PluginHost::IShell* service) {
    _service = service;

    // Load configuration
    string configLine = service->ConfigLine();
    Config config;
    if (!configLine.empty()) {
        config.FromString(configLine);
    }
    _serverPort = static_cast<uint16_t>(config.serverPort.Value());
    _permissionEnforcement = config.permissionEnforcement.Value();
    _jwtEnabled = config.jwtEnabled.Value();

    // Acquire SecurityAgent token if available (pattern placeholder)
    // In a full implementation, query SecurityAgent and store token in _securityToken
    _securityToken.clear();

    _router.reset(new RequestRouter(service, _securityToken));
    _perms.reset(new PermissionManager(_jwtEnabled));

    // Load initial resolutionPaths
    std::vector<string> initialPaths;
    Core::JSON::ArrayType<Core::JSON::String>::Iterator it(config.resolutionPaths.Elements());
    while (it.Next() == true) {
        initialPaths.emplace_back(it.Current().Value());
    }
    if (!initialPaths.empty()) {
        string error;
        _resolver->LoadPaths(initialPaths, error);
    }

    // Start WebSocket server if configured
    if (_serverPort != 0) {
        string error;
        if (!_ws->Start(_serverPort, error)) {
            return string(_T("AppGateway: Failed to start WebSocket server"));
        }
    }

    _running = true;
    return string();
}

void AppGateway::Deinitialize(PluginHost::IShell* /*service*/) {
    _running = false;
    if (_ws) {
        _ws->Stop();
    }
    _router.reset();
    _perms.reset();
    _resolver.reset();
    _connections.reset();
    _service = nullptr;
}

string AppGateway::Information() const {
    // Provide plugin description
    return string(_T("AppGateway: Resolves Firebolt methods to Thunder aliases and provides respond/resolve/configure APIs."));
}

uint32_t AppGateway::ParseConfigureParams(const Core::JSON::Object& paramsObject,
                                          Core::JSON::ArrayType<Core::JSON::String>& outPaths) const {
    if (paramsObject.HasLabel(kPaths) == false) {
        return static_cast<uint32_t>(~0); // INVALID_REQUEST equivalent
    }
    const Core::JSON::ArrayType<Core::JSON::String>& paths = paramsObject.Get<Core::JSON::ArrayType<Core::JSON::String>>(kPaths);
    outPaths = paths;
    return Core::ERROR_NONE;
}

uint32_t AppGateway::ParseResolveParams(const Core::JSON::Object& paramsObject,
                                        Core::JSON::Object& outContext,
                                        string& outMethod,
                                        Core::JSON::Object& outParams) const {
    if ((paramsObject.HasLabel(kContext) == false) || (paramsObject.HasLabel(kMethod) == false)) {
        return Core::ERROR_BAD_REQUEST; // INVALID_PARAMS
    }
    outContext = paramsObject.Get<Core::JSON::Object>(kContext);
    Core::JSON::String method = paramsObject.Get<Core::JSON::String>(kMethod);
    outMethod = method.Value();
    if (paramsObject.HasLabel(kParams) == true) {
        outParams = paramsObject.Get<Core::JSON::Object>(kParams);
    } else {
        outParams.Clear();
    }
    // Validate context has appId
    if (!outContext.HasLabel(kAppId)) {
        return Core::ERROR_BAD_REQUEST;
    }
    return Core::ERROR_NONE;
}

uint32_t AppGateway::ParseRespondParams(const Core::JSON::Object& paramsObject,
                                        Core::JSON::Object& outContext,
                                        Core::JSON::Object& outPayload) const {
    if (paramsObject.HasLabel(kContext) == false) {
        return Core::ERROR_BAD_REQUEST;
    }
    outContext = paramsObject.Get<Core::JSON::Object>(kContext);

    // payload can be "payload", or legacy "result"/"error" at top-level within params
    if (paramsObject.HasLabel(kPayload) == true) {
        outPayload = paramsObject.Get<Core::JSON::Object>(kPayload);
    } else {
        // Build payload wrapper if legacy fields exist
        Core::JSON::Object wrapper;
        if (paramsObject.HasLabel(kResult) == true) {
            wrapper.Set(kResult, paramsObject.Get<Core::JSON::Object>(kResult));
        }
        if (paramsObject.HasLabel(kError) == true) {
            wrapper.Set(kError, paramsObject.Get<Core::JSON::Object>(kError));
        }
        if (wrapper.Length() == 0) {
            return Core::ERROR_INCOMPLETE_CONFIG; // Use as generic invalid params
        }
        outPayload = wrapper;
    }

    // Validate context fields
    if (!outContext.HasLabel(kRequestId) || !outContext.HasLabel(kConnectionId) || !outContext.HasLabel(kAppId)) {
        return Core::ERROR_BAD_REQUEST;
    }
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
