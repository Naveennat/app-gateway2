/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
 
#pragma once

// Module: OttServices
// This header declares the OttServices Thunder plugin public interface and JSON-RPC API.
// Follows naming and structural conventions similar to AppNotifications plugin in this codebase.

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IPlugin.h>
#include <plugins/JSONRPC.h>
#include <plugins/IShell.h>
#include <atomic>
#include <type_traits>
#include <utility>

#include  <interfaces/IOttServices.h>

namespace WPEFramework {

namespace Plugin {

    class OttServices : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        OttServices(const OttServices&) = delete;
        OttServices& operator=(const OttServices&) = delete;

    public:
        OttServices();
        ~OttServices() override;

        /** Initialize the plugin and acquire the out-of-process IOttServices interface. */
        const string Initialize(PluginHost::IShell* shell) override;

        /** Release the acquired interface and terminate remote connection if needed. */
        void Deinitialize(PluginHost::IShell* service) override;

        /** Provide plugin information (empty for thin adapter). */
        string Information() const override { return {}; }

        BEGIN_INTERFACE_MAP(OttServices)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IOttServices, _mOttServices)
        END_INTERFACE_MAP

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        PluginHost::IShell* _service;
        Exchange::IOttServices* _mOttServices;
        uint32_t _connectionId;
    };

} // namespace Plugin
} // namespace WPEFramework
