#pragma once

/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Thin JSON-RPC adapter plugin for delegating calls to an out-of-process
 * IOttServices implementation. Pattern mirrors FbSettings.
 */

#include "Module.h"
#include <interfaces/IOttServices.h>

namespace WPEFramework {

namespace Plugin {

    // This is a server for a JSONRPC communication channel.
    // For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
    class OttServices : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        OttServices(const OttServices&) = delete;
        OttServices& operator=(const OttServices&) = delete;

    public:
        OttServices();
        ~OttServices() override;

        // PUBLIC_INTERFACE
        const string Initialize(PluginHost::IShell* shell) override;
        /** Initialize the plugin and acquire the out-of-process IOttServices interface. */

        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service) override;
        /** Release the acquired interface and terminate remote connection if needed. */

        // PUBLIC_INTERFACE
        string Information() const override { return {}; }
        /** Provide plugin information (empty for thin adapter). */

        BEGIN_INTERFACE_MAP(OttServices)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IOttServices, _ottServices)
        END_INTERFACE_MAP

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        PluginHost::IShell* _service;
        Exchange::IOttServices* _ottServices;
        uint32_t _connectionId;
    };

} // namespace Plugin
} // namespace WPEFramework
