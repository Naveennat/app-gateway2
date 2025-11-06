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

#include "Module.h"
#include <interfaces/IXvpClient.h>

#include "UtilsLogging.h"


namespace WPEFramework {

    namespace Plugin {


		// This is a server for a JSONRPC communication channel.
		// For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
		// By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
		// This realization of this interface implements, by default, the following methods on this plugin
		// - exists
		// - register
		// - unregister
		// Any other methood to be handled by this plugin  can be added can be added by using the
		// templated methods Register on the PluginHost::JSONRPC class.
		// As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
		// this class exposes a public method called, Notify(), using this methods, all subscribed clients
		// will receive a JSONRPC message as a notification, in case this method is called.
        class XvpClient : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        private:
            // We do not allow this plugin to be copied !!
            XvpClient(const XvpClient&) = delete;
            XvpClient& operator=(const XvpClient&) = delete;

        public:
            XvpClient();
            virtual ~XvpClient();
            virtual const string Initialize(PluginHost::IShell* shell) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }

            BEGIN_INTERFACE_MAP(XvpClient)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IXvpSession, mXvpClient)
            INTERFACE_AGGREGATE(Exchange::IXvpPlayback, mXvpClient)
            INTERFACE_AGGREGATE(Exchange::IXvpXifa, mXvpClient)
            INTERFACE_AGGREGATE(Exchange::IXvpVideo, mXvpClient)
            END_INTERFACE_MAP

            class Notification : public RPC::IRemoteConnection::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification &) = delete;
                Notification &operator=(const Notification &) = delete;

            public:
                explicit Notification(XvpClient *parent) : mParent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification() override
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection *) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                    mParent.Deactivated(connection);
                }

            private:
                XvpClient &mParent;
            };

        void Deactivated(RPC::IRemoteConnection* connection);

        private:
            PluginHost::IShell* mService;
            WPEFramework::Core::IUnknown* mXvpClient;
            uint32_t mConnectionId;
            Core::Sink<Notification> mNotification;
        };
	} // namespace Plugin
} // namespace WPEFramework