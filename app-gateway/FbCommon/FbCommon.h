#pragma once

#include "Module.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/IAppGateway.h>

namespace WPEFramework {

namespace Plugin {

    // PUBLIC_INTERFACE
    class FbCommon : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        /** FbCommon wrapper plugin that aggregates a remote implementation exposing
         *  Exchange::IAppGatewayRequestHandler. This mirrors the FbSettings pattern:
         *  - PluginHost::IPlugin + JSONRPC wrapper
         *  - Aggregates underlying COM object created via service->Root(...)
         *  - Forwards QueryInterface requests (e.g., IAppGatewayRequestHandler) through aggregation
         */
    private:
        FbCommon(const FbCommon&) = delete;
        FbCommon& operator=(const FbCommon&) = delete;

    public:
        FbCommon();
        ~FbCommon() override;
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override { return {}; }

        BEGIN_INTERFACE_MAP(FbCommon)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            // Aggregate the request handler so AppGateway can acquire Exchange::IAppGatewayRequestHandler
            INTERFACE_AGGREGATE(Exchange::IAppGatewayRequestHandler, mRequestHandler)
        END_INTERFACE_MAP

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        PluginHost::IShell* mService;
        Exchange::IAppGatewayRequestHandler* mRequestHandler;
        uint32_t mConnectionId;
    };

} // namespace Plugin
} // namespace WPEFramework
