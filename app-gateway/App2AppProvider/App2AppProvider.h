#pragma once

#include "Module.h"
#include "ProviderRegistry.h"
#include "CorrelationMap.h"
#include "GatewayClient.h"
#include "PermissionManager.h"

namespace WPEFramework {
namespace Plugin {

/**
 * App2AppProvider
 * Implements provider/consumer orchestration:
 * - registerProvider
 * - invokeProvider
 * - handleProviderResponse
 * - handleProviderError
 */
class App2AppProvider : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    class Config : public Core::JSON::Container {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        Config()
            : Core::JSON::Container()
            , JwtEnabled(false)
            , GatewayCallsign("org.rdk.AppGateway")
            , ProviderConflictPolicy("lastWins") {
            Add(_T("jwtEnabled"), &JwtEnabled);
            Add(_T("gatewayCallsign"), &GatewayCallsign);
            Add(_T("providerConflictPolicy"), &ProviderConflictPolicy);
        }

        Core::JSON::Boolean JwtEnabled;
        Core::JSON::String GatewayCallsign;
        Core::JSON::String ProviderConflictPolicy;
    };

public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

    // IPlugin methods
    // PUBLIC_INTERFACE
    const string Initialize(PluginHost::IShell* service) override;
    // PUBLIC_INTERFACE
    void Deinitialize(PluginHost::IShell* service) override;
    // PUBLIC_INTERFACE
    string Information() const override;

private:
    // JSON-RPC parameter containers
    class Context : public Core::JSON::Container {
    public:
        Context() {
            Add(_T("requestId"), &RequestId);
            Add(_T("connectionId"), &ConnectionId);
            Add(_T("appId"), &AppId);
        }
        Core::JSON::DecUInt32 RequestId;
        Core::JSON::String ConnectionId;
        Core::JSON::String AppId;
    };

    class RegisterProviderParams : public Core::JSON::Container {
    public:
        RegisterProviderParams() {
            Add(_T("context"), &Ctx);
            Add(_T("register"), &Register);
            Add(_T("capability"), &Capability);
        }
        Context Ctx;
        Core::JSON::Boolean Register;
        Core::JSON::String Capability;
    };

    class InvokeProviderParams : public Core::JSON::Container {
    public:
        InvokeProviderParams() {
            Add(_T("context"), &Ctx);
            Add(_T("capability"), &Capability);
            Add(_T("payload"), &Payload);
        }
        Context Ctx;
        Core::JSON::String Capability;
        Core::JSON::Object Payload;
    };

    class InvokeProviderResult : public Core::JSON::Container {
    public:
        InvokeProviderResult() {
            Add(_T("correlationId"), &CorrelationId);
        }
        Core::JSON::String CorrelationId;
    };

    class ProviderResponseParams : public Core::JSON::Container {
    public:
        ProviderResponseParams() {
            Add(_T("payload"), &Payload);
            Add(_T("capability"), &Capability);
        }
        Core::JSON::Object Payload;  // includes correlationId + result|error
        Core::JSON::String Capability;
    };

private:
    // JSON-RPC handlers
    uint32_t endpoint_registerProvider(const RegisterProviderParams& params, Core::JSON::Container& response);
    uint32_t endpoint_invokeProvider(const InvokeProviderParams& params, InvokeProviderResult& response);
    uint32_t endpoint_handleProviderResponse(const ProviderResponseParams& params, Core::JSON::Container& response);
    uint32_t endpoint_handleProviderError(const ProviderResponseParams& params, Core::JSON::Container& response);

    void RegisterMethods();
    void UnregisterMethods();

    bool ExtractSecurityToken(PluginHost::IShell* service, string& token) const;
    bool ValidateContext(const Context& ctx) const;

private:
    PluginHost::IShell* _service;
    std::unique_ptr<ProviderRegistry> _providers;
    std::unique_ptr<CorrelationMap> _correlations;
    std::unique_ptr<GatewayClient> _gateway;
    std::unique_ptr<PermissionManager> _perms;

    string _gatewayCallsign;
    bool _jwtEnabled;
    string _conflictPolicy;
    string _securityToken;
};

} // namespace Plugin
} // namespace WPEFramework
