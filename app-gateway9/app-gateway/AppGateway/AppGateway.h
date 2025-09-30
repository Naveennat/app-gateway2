#pragma once

#include "Module.h"
#include <plugins/JSONRPC.h>
#include <plugins/IPlugin.h>
#include <core/JSON.h>
#include <core/Enumerate.h>
#include <mutex>
#include <atomic>
#include <memory>
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {

// Forward declarations
class Resolver;
class RequestRouter;
class ConnectionRegistry;
class PermissionManager;
class GatewayWebSocket;

// PUBLIC_INTERFACE
class AppGateway : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    /*
     * AppGateway Plugin
     *
     * Exposes JSON-RPC methods to configure resolution overlays,
     * resolve Firebolt-style methods into Thunder aliases, and respond to
     * specific application contexts over WebSocket (via GatewayWebSocket).
     *
     * Methods (versioned callsign recommended: org.rdk.AppGateway.1):
     *  - configure(paths: array<string>) -> null
     *  - resolve(context: {appId: string}, method: string, params?: object) -> { resolution: object }
     *  - respond(context: {requestId:number, connectionId:string, appId:string}, payload: object) -> null
     */
    AppGateway();
    AppGateway(const AppGateway&) = delete;
    AppGateway& operator=(const AppGateway&) = delete;

    ~AppGateway() override;

    // Provide QueryInterface mapping for WPEFramework services
    BEGIN_INTERFACE_MAP(AppGateway)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
    END_INTERFACE_MAP

    // IPlugin
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
    string Information() const override;

public:
    // JSON-RPC method handlers (return Core::ERROR_* codes)
    // PUBLIC_INTERFACE
    uint32_t configure(const Core::JSON::ArrayType<Core::JSON::String>& paths);

    // PUBLIC_INTERFACE
    uint32_t resolve(const Core::JSON::VariantContainer& params, Core::JSON::VariantContainer& result);

    // PUBLIC_INTERFACE
    uint32_t respond(const Core::JSON::VariantContainer& params);

private:
    // Internal helpers to parse params safely
    uint32_t ParseConfigureParams(const Core::JSON::VariantContainer& paramsObject,
                                  Core::JSON::ArrayType<Core::JSON::String>& outPaths) const;

    uint32_t ParseResolveParams(const Core::JSON::VariantContainer& paramsObject,
                                Core::JSON::VariantContainer& outContext,
                                string& outMethod,
                                Core::JSON::VariantContainer& outParams) const;

    uint32_t ParseRespondParams(const Core::JSON::VariantContainer& paramsObject,
                                Core::JSON::VariantContainer& outContext,
                                Core::JSON::VariantContainer& outPayload) const;

private:
    PluginHost::IShell* _service;
    std::unique_ptr<Resolver> _resolver;
    std::unique_ptr<RequestRouter> _router;
    std::unique_ptr<ConnectionRegistry> _connections;
    std::unique_ptr<PermissionManager> _perms;
    std::unique_ptr<GatewayWebSocket> _ws;
    std::atomic<bool> _running;
    string _securityToken;

    // Configuration values
    uint16_t _serverPort;
    bool _permissionEnforcement;
    bool _jwtEnabled;

private:
    // Configuration structure to deserialize plugin config
    class Config : public Core::JSON::Container {
    public:
        Config()
            : Core::JSON::Container()
            , serverPort(3473)
            , permissionEnforcement(true)
            , jwtEnabled(false)
            , resolutionPaths() {
            Add(_T("serverPort"), &serverPort);
            Add(_T("permissionEnforcement"), &permissionEnforcement);
            Add(_T("jwtEnabled"), &jwtEnabled);
            Add(_T("resolutionPaths"), &resolutionPaths);
        }

        Core::JSON::DecUInt16 serverPort;
        Core::JSON::Boolean permissionEnforcement;
        Core::JSON::Boolean jwtEnabled;
        Core::JSON::ArrayType<Core::JSON::String> resolutionPaths;
    };

};

} // namespace Plugin
} // namespace WPEFramework
