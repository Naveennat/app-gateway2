#include "App2AppProvider.h"

using namespace WPEFramework;
using namespace WPEFramework::Core;
using namespace WPEFramework::Plugin;

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {
namespace {

static Plugin::Metadata<Plugin::App2AppProvider> metadata(
    // Version
    API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
    // Preconditions
    {},
    // Terminations
    {},
    // Controls
    {}
);

} // anonymous

namespace Plugin {

SERVICE_REGISTRATION(App2AppProvider, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

App2AppProvider::App2AppProvider()
    : PluginHost::JSONRPC()
    , _service(nullptr)
    , _providers()
    , _correlations()
    , _gateway()
    , _perms()
    , _gatewayCallsign("org.rdk.AppGateway") {

    // Register JSON-RPC methods
    Register<JsonObject, JsonObject>(_T("registerProvider"), &App2AppProvider::RegisterProviderWrapper, this);
    Register<JsonObject, JsonObject>(_T("invokeProvider"), &App2AppProvider::InvokeProviderWrapper, this);
    Register<JsonObject, JsonObject>(_T("handleProviderResponse"), &App2AppProvider::HandleProviderResponseWrapper, this);
    Register<JsonObject, JsonObject>(_T("handleProviderError"), &App2AppProvider::HandleProviderErrorWrapper, this);

    LOGINFO("App2AppProvider constructed and JSON-RPC methods registered");
}

App2AppProvider::~App2AppProvider() {
    // Unregister JSON-RPC methods
    Unregister(_T("registerProvider"));
    Unregister(_T("invokeProvider"));
    Unregister(_T("handleProviderResponse"));
    Unregister(_T("handleProviderError"));

    LOGINFO("App2AppProvider destructed and JSON-RPC methods unregistered");
}

const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    LOGINFO("Initialize called");
    _service = service;

    // Acquire SecurityAgent token for COMRPC local dispatch
    std::string token;
    if (_service != nullptr) {
        auto security = _service->QueryInterfaceByCallsign<PluginHost::IAuthenticate>("SecurityAgent");
        if (security != nullptr) {
            string payload = "http://localhost";
            if (security->CreateToken(static_cast<uint16_t>(payload.length()),
                                      reinterpret_cast<const uint8_t*>(payload.c_str()),
                                      token) == Core::ERROR_NONE) {
                LOGINFO("App2AppProvider obtained SecurityAgent token");
            } else {
                LOGWARN("App2AppProvider failed to obtain SecurityAgent token");
            }
            security->Release();
        } else {
            LOGWARN("App2AppProvider: SecurityAgent interface not available");
        }
    }

    // Construct helpers
    _providers = std::unique_ptr<App2App::ProviderRegistry>(new App2App::ProviderRegistry());
    _correlations = std::unique_ptr<App2App::CorrelationMap>(new App2App::CorrelationMap());
    _gateway = std::unique_ptr<App2App::GatewayClient>(new App2App::GatewayClient(_service, _gatewayCallsign, token));
    _perms = std::unique_ptr<App2App::PermissionManager>(new App2App::PermissionManager());

    LOGINFO("App2AppProvider initialized successfully");
    return string();
}

void App2AppProvider::Deinitialize(PluginHost::IShell* /*service*/) {
    LOGINFO("Deinitialize called");
    _gateway.reset();
    _perms.reset();
    _correlations.reset();
    _providers.reset();
    _service = nullptr;
}

string App2AppProvider::Information() const {
    return string("{\"service\":\"org.rdk.App2AppProvider\"}");
}

bool App2AppProvider::ValidateContext(const Core::JSON::JsonObject& context, std::string& appId, std::string& connectionId, uint32_t& requestId) const {
    if (!context.HasLabel("appId") || !context.HasLabel("connectionId") || !context.HasLabel("requestId")) {
        return false;
    }
    appId = context.Get("appId").String();
    connectionId = context.Get("connectionId").String();
    requestId = static_cast<uint32_t>(context.Get("requestId").Number());
    return !(appId.empty() || connectionId.empty());
}

bool App2AppProvider::ValidateStringField(const Core::JSON::JsonObject& object, const std::string& field, std::string& out) const {
    if (!object.HasLabel(field.c_str())) return false;
    out = object.Get(field.c_str()).String();
    return !out.empty();
}

uint32_t App2AppProvider::RegisterProviderWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response) {
    LOGINFOMETHOD();
    bool success = false;

    do {
        if (_providers == nullptr) {
            response["message"] = "Provider registry unavailable";
            break;
        }

        // Extract required fields
        if (!parameters.HasLabel("context") || !parameters.HasLabel("register") || !parameters.HasLabel("capability")) {
            response["message"] = "Missing required parameters (context/register/capability)";
            break;
        }

        const auto& context = parameters.Get("context").Object();
        std::string appId, connectionId, capability;
        uint32_t requestId = 0;

        if (!ValidateContext(context, appId, connectionId, requestId)) {
            response["message"] = "Invalid context";
            break;
        }
        capability = parameters.Get("capability").String();
        if (capability.empty()) {
            response["message"] = "Invalid capability";
            break;
        }

        bool doRegister = parameters.Get("register").Boolean();

        if (doRegister) {
            App2App::ProviderContext pctx;
            pctx.appId = appId;
            pctx.connectionId = connectionId;

            success = _providers->Register(capability, pctx);
            if (!success) {
                response["message"] = "Registration failed (policy rejected or internal error)";
            }
        } else {
            success = _providers->Unregister(capability, appId);
            if (!success) {
                response["message"] = "Unregister failed (not owner or not found)";
            }
        }
    } while (false);

    returnResponse(success);
}

uint32_t App2AppProvider::InvokeProviderWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response) {
    LOGINFOMETHOD();
    bool success = false;

    do {
        if ((_providers == nullptr) || (_correlations == nullptr)) {
            response["message"] = "Internal components unavailable";
            break;
        }

        if (!parameters.HasLabel("context") || !parameters.HasLabel("capability")) {
            response["message"] = "Missing required parameters (context/capability)";
            break;
        }

        const auto& context = parameters.Get("context").Object();
        std::string consumerAppId, consumerConnectionId, capability;
        uint32_t consumerRequestId = 0;
        if (!ValidateContext(context, consumerAppId, consumerConnectionId, consumerRequestId)) {
            response["message"] = "Invalid context";
            break;
        }

        capability = parameters.Get("capability").String();
        if (capability.empty()) {
            response["message"] = "Invalid capability";
            break;
        }

        // Resolve provider for capability
        App2App::ProviderContext providerCtx;
        if (!_providers->Resolve(capability, providerCtx)) {
            response["message"] = "PROVIDER_NOT_FOUND";
            break;
        }

        // Record correlation between correlationId and consumer context
        App2App::ConsumerContext consumerCtx;
        consumerCtx.appId = consumerAppId;
        consumerCtx.connectionId = consumerConnectionId;
        consumerCtx.requestId = consumerRequestId;

        const std::string correlationId = _correlations->Create(consumerCtx);
        if (correlationId.empty()) {
            response["message"] = "Failed to create correlation";
            break;
        }

        // Return correlationId to caller (AppGateway will forward the provider request using resolution config)
        Core::JSON::String corr(correlationId);
        response.Set("correlationId", corr);
        success = true;
    } while (false);

    returnResponse(success);
}

uint32_t App2AppProvider::HandleProviderResponseWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response) {
    LOGINFOMETHOD();
    bool success = false;

    do {
        if ((_correlations == nullptr) || (_gateway == nullptr)) {
            response["message"] = "Internal components unavailable";
            break;
        }

        if (!parameters.HasLabel("payload") || !parameters.HasLabel("capability")) {
            response["message"] = "Missing required parameters (payload/capability)";
            break;
        }

        const auto& payload = parameters.Get("payload").Object();
        if (!payload.HasLabel("correlationId") || !payload.HasLabel("result")) {
            response["message"] = "Invalid payload (missing correlationId/result)";
            break;
        }

        std::string correlationId = payload.Get("correlationId").String();
        if (correlationId.empty()) {
            response["message"] = "Empty correlationId";
            break;
        }

        App2App::ConsumerContext consumerCtx;
        if (!_correlations->Take(correlationId, consumerCtx)) {
            response["message"] = "UNKNOWN_CORRELATION";
            break;
        }

        // Build payload to forward back to consumer via AppGateway.respond
        Core::JSON::JsonObject forwardPayload;
        forwardPayload.Set("result", payload.Get("result"));

        const uint32_t status = _gateway->Respond(consumerCtx, forwardPayload);
        if (status != Core::ERROR_NONE) {
            response["message"] = "Failed to forward result to consumer";
            break;
        }

        success = true;
    } while (false);

    returnResponse(success);
}

uint32_t App2AppProvider::HandleProviderErrorWrapper(const Core::JSON::JsonObject& parameters, Core::JSON::JsonObject& response) {
    LOGINFOMETHOD();
    bool success = false;

    do {
        if ((_correlations == nullptr) || (_gateway == nullptr)) {
            response["message"] = "Internal components unavailable";
            break;
        }

        if (!parameters.HasLabel("payload") || !parameters.HasLabel("capability")) {
            response["message"] = "Missing required parameters (payload/capability)";
            break;
        }

        const auto& payload = parameters.Get("payload").Object();
        if (!payload.HasLabel("correlationId") || !payload.HasLabel("error")) {
            response["message"] = "Invalid payload (missing correlationId/error)";
            break;
        }

        std::string correlationId = payload.Get("correlationId").String();
        if (correlationId.empty()) {
            response["message"] = "Empty correlationId";
            break;
        }

        App2App::ConsumerContext consumerCtx;
        if (!_correlations->Take(correlationId, consumerCtx)) {
            response["message"] = "UNKNOWN_CORRELATION";
            break;
        }

        // Build error payload and forward
        Core::JSON::JsonObject forwardPayload;
        forwardPayload.Set("error", payload.Get("error"));

        const uint32_t status = _gateway->Respond(consumerCtx, forwardPayload);
        if (status != Core::ERROR_NONE) {
            response["message"] = "Failed to forward error to consumer";
            break;
        }

        success = true;
    } while (false);

    returnResponse(success);
}

} // namespace Plugin
} // namespace WPEFramework
