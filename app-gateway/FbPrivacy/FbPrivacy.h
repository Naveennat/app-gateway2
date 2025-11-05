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
#include <interfaces/IFbPrivacy.h>
#include <interfaces/IAppNotifications.h>
#include <interfaces/json/JsonData_FbPrivacy.h>
#include <interfaces/json/JFbPrivacy.h>

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
        class FbPrivacy : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        private:
            // We do not allow this plugin to be copied !!
            FbPrivacy(const FbPrivacy&) = delete;
            FbPrivacy& operator=(const FbPrivacy&) = delete;

        public:
            FbPrivacy();
            virtual ~FbPrivacy();
            virtual const string Initialize(PluginHost::IShell* shell) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }

            BEGIN_INTERFACE_MAP(FbPrivacy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IFbPrivacy, mFbPrivacy)
            INTERFACE_AGGREGATE(Exchange::IAppNotificationHandler, mAppNotificationHandler)
            END_INTERFACE_MAP

        private:

        class Notification : public RPC::IRemoteConnection::INotification,
                                 public Exchange::IFbPrivacy::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification &) = delete;
                Notification &operator=(const Notification &) = delete;

            public:
                explicit Notification(FbPrivacy *parent) : mParent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification() override
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(Exchange::IFbPrivacy::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection *) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                    mParent.Deactivated(connection);
                }

                void OnAllowACRCollectionChanged(const bool value) override
                {
                    LOGINFO("OnAllowACRCollectionChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowACRCollectionChanged(mParent, value);
                }

                void OnAllowAppContentAdTargetingChanged(const bool value) override
                {
                    LOGINFO("OnAllowAppContentAdTargetingChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowAppContentAdTargetingChanged(mParent, value);
                }
                
                void OnAllowCameraAnalyticsChanged(const bool value) override
                {
                    LOGINFO("OnAllowCameraAnalyticsChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowCameraAnalyticsChanged(mParent, value);
                }

                void OnAllowPersonalizationChanged(const bool value) override
                {
                    LOGINFO("OnAllowPersonalizationChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowPersonalizationChanged(mParent, value);
                }

                void OnAllowPrimaryBrowseAdTargetingChanged(const bool value) override
                {
                    LOGINFO("OnAllowPrimaryBrowseAdTargetingChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowPrimaryBrowseAdTargetingChanged(mParent, value);
                }

                void OnAllowPrimaryContentAdTargetingChanged(const bool value) override
                {
                    LOGINFO("OnAllowPrimaryContentAdTargetingChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowPrimaryContentAdTargetingChanged(mParent, value);
                }

                void OnAllowProductAnalyticsChanged(const bool value) override
                {
                    LOGINFO("OnAllowProductAnalyticsChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowProductAnalyticsChanged(mParent, value);
                }

                void OnAllowRemoteDiagnosticsChanged(const bool value) override
                {
                    LOGINFO("OnAllowRemoteDiagnosticsChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowRemoteDiagnosticsChanged(mParent, value);
                }

                void OnAllowResumePointsChanged(const bool value) override
                {
                    LOGINFO("OnAllowResumePointsChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowResumePointsChanged(mParent, value);
                }

                void OnAllowUnentitledPersonalizationChanged(const bool value) override
                {
                    LOGINFO("OnAllowUnentitledPersonalizationChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowUnentitledPersonalizationChanged(mParent, value);
                }

                void OnAllowUnentitledResumePointsChanged(const bool value) override
                {
                    LOGINFO("OnAllowUnentitledResumePointsChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowUnentitledResumePointsChanged(mParent, value);
                }

                void OnAllowWatchHistoryChanged(const bool value) override
                {
                    LOGINFO("OnAllowWatchHistoryChanged\n");
                    Exchange::JFbPrivacy::Event::OnAllowWatchHistoryChanged(mParent, value);
                }

            private:
                FbPrivacy &mParent;
            };


            void Deactivated(RPC::IRemoteConnection* connection);

        private:
            PluginHost::IShell* mService;
            WPEFramework::Core::IUnknown* mPluginHandler;
            Exchange::IFbPrivacy* mFbPrivacy;
            Exchange::IAppNotificationHandler* mAppNotificationHandler;
            uint32_t mConnectionId;
            Core::Sink<Notification> mNotification;
        };
	} // namespace Plugin
} // namespace WPEFramework
