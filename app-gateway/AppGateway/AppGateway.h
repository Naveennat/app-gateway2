#pragma once

#include "Module.h"
#include "Resolver.h"
#include "RequestRouter.h"
#include "ConnectionRegistry.h"
#include "PermissionManager.h"
#include "GatewayWebSocket.h"

namespace WPEFramework {
namespace Plugin {

class AppGateway : public PluginHost::IPlugin, public PluginHost::JSONRPC {
public:
    class Config : public Core::JSON::Container {
    public:
        Config(const Config&) = delete;
        Config& operator=(const Config&) = delete;

        Config()
            : Core::JSON::Container()
            , ServerPort(3473)
            , PermissionEnforcement(true)
            , JwtEnabled(false)
            , GatewayCallsign("org.rdk.AppGateway") {
            Add(_T("serverPort"), &ServerPort);
            Add(_T("permissionEnforcement"), &PermissionEnforcement);
            Add(_T("jwtEnabled"), &JwtEnabled);
            Add(_T("resolutionPaths"), &ResolutionPaths);
            Add(_T("gatewayCallsign"), &GatewayCallsign);
        }

        Core::JSON::DecUInt16 ServerPort;
        Core::JSON::Boolean PermissionEnforcement;
        Core::JSON::Boolean JwtEnabled;
        Core::JSON::ArrayType<Core::JSON::String> ResolutionPaths;
        Core::JSON::String GatewayCallsign;
    };

public:
    AppGateway(const AppGateway&) = delete;
    AppGateway& operator=(const AppGateway&) = delete;

    AppGateway();
    ~AppGateway() override;

    // PUBLIC_INTERFACE
    uint32_t AddRef() const override;
    /** Increase the reference count for the plugin instance. */

    // PUBLIC_INTERFACE
    uint32_t Release() const override;
    /** Decrease the reference count; deletes object when it reaches zero. */

    // PUBLIC_INTERFACE
    void* QueryInterface(const uint32_t id) override;
    /** Query supported interfaces (IPlugin and IDispatcher). */

    // IPlugin methods
    const string Initialize(PluginHost::IShell* service) override;
    void Deinitialize(PluginHost::IShell* service) override;
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

    class ConfigureParams : public Core::JSON::Container {
    public:
        ConfigureParams() {
            Add(_T("paths"), &Paths);
        }
        Core::JSON::ArrayType<Core::JSON::String> Paths;
    };

    class RespondParams : public Core::JSON::Container {
    public:
        RespondParams() {
            Add(_T("context"), &Ctx);
            Add(_T("payload"), &Payload);
            // For legacy-compatibility: accept top-level result/error (ignored if payload present)
            Add(_T("result"), &Result);
            Add(_T("error"), &Error);
        }
        Context Ctx;
        Core::JSON::Object Payload;
        Core::JSON::Object Result;
        Core::JSON::Object Error;
    };

    class ResolveParams : public Core::JSON::Container {
    public:
        ResolveParams() {
            Add(_T("context"), &Ctx); // only appId is required here per design
            Add(_T("method"), &Method);
            Add(_T("params"), &Params);
        }
        Core::JSON::Object Ctx;
        Core::JSON::String Method;
        Core::JSON::Object Params;
    };

    class ResolveResult : public Core::JSON::Container {
    public:
        ResolveResult() {
            Add(_T("resolution"), &Resolution);
        }
        Core::JSON::Object Resolution;
    };

private:
    // JSON-RPC handlers
    uint32_t endpoint_configure(const ConfigureParams& params, Core::JSON::Container& response);
    uint32_t endpoint_respond(const RespondParams& params, Core::JSON::Container& response);
    uint32_t endpoint_resolve(const ResolveParams& params, ResolveResult& response);

    void RegisterMethods();
    void UnregisterMethods();

    bool ExtractSecurityToken(PluginHost::IShell* service, string& token) const;

private:
    mutable std::atomic<uint32_t> _refCount;
    PluginHost::IShell* _service;
    std::unique_ptr<Resolver> _resolver;
    std::unique_ptr<RequestRouter> _router;
    std::unique_ptr<ConnectionRegistry> _connections;
    std::unique_ptr<PermissionManager> _perms;
    std::unique_ptr<GatewayWebSocket> _ws;

    string _securityToken;
    uint16_t _wsPort;
    bool _permissionEnforcement;
    bool _jwtEnabled;
    string _callsign;
};

} // namespace Plugin
} // namespace WPEFramework
