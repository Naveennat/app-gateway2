#pragma once

#include "Module.h"
#include <plugins/JSONRPC.h>
#include <plugins/IPlugin.h>
#include <core/JSON.h>
#include <memory>
#include <atomic>

#include "ProviderRegistry.h"
#include "CorrelationMap.h"
#include "GatewayClient.h"
#include "PermissionManager.h"

namespace WPEFramework {
namespace Plugin {

// PUBLIC_INTERFACE
class App2AppProvider : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    /**
     * App2AppProvider JSON-RPC Plugin
     *
     * Summary:
     *   Enables provider/consumer interactions between applications by registering
     *   providers for capabilities, handling invocations on behalf of consumers,
     *   and routing provider responses and errors back to the original consumer
     *   via AppGateway.respond.
     *
     * Versioned call sign (recommended): App2AppProvider.1
     * Methods:
     *  - registerProvider(context: {requestId:number, connectionId:string, appId:string}, register:boolean, capability:string) -> null
     *  - invokeProvider(context: {requestId:number, connectionId:string, appId:string}, capability:string, payload?:object) -> { correlationId:string }
     *  - handleProviderResponse(payload:{ correlationId:string, result:object }, capability:string) -> null
     *  - handleProviderError(payload:{ correlationId:string, error:{code:number,message:string}}, capability:string) -> null
     */
    App2AppProvider();
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;
    ~App2AppProvider() override;

    // Provide QueryInterface mapping for WPEFramework services
    BEGIN_INTERFACE_MAP(App2AppProvider)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    // IPlugin
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

public:
    // PUBLIC_INTERFACE
    uint32_t registerProvider(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);

    // PUBLIC_INTERFACE
    uint32_t invokeProvider(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);

    // PUBLIC_INTERFACE
    uint32_t handleProviderResponse(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);

    // PUBLIC_INTERFACE
    uint32_t handleProviderError(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& response);

private:
    // Helpers for parsing and validation
    bool ParseContext(const Core::JSON::VariantContainer& obj, ConsumerContext& outCtx, string& error) const;
    bool ParseProviderContext(const Core::JSON::VariantContainer& obj, ProviderContext& outCtx, string& error) const;

private:
    PluginHost::IShell* _service;
    std::unique_ptr<ProviderRegistry> _providers;
    std::unique_ptr<CorrelationMap> _correlations;
    std::unique_ptr<GatewayClient> _gateway;
    std::unique_ptr<PermissionManager> _perms;
    std::atomic<bool> _running;

    // Configuration values
    string _gatewayCallsign;
    bool _jwtEnabled;
    string _policy; // "lastWins" | "rejectDuplicates"

private:
    // Configuration structure to deserialize plugin config
    class Config : public Core::JSON::Container {
    public:
        Config()
            : Core::JSON::Container()
            , gatewayCallsign(_T("AppGateway"))
            , jwtEnabled(false)
            , providerConflictPolicy(_T("lastWins")) {
            Add(_T("gatewayCallsign"), &gatewayCallsign);
            Add(_T("jwtEnabled"), &jwtEnabled);
            Add(_T("providerConflictPolicy"), &providerConflictPolicy);
        }

        Core::JSON::String gatewayCallsign;
        Core::JSON::Boolean jwtEnabled;
        Core::JSON::String providerConflictPolicy;
    };
};

} // namespace Plugin
} // namespace WPEFramework
