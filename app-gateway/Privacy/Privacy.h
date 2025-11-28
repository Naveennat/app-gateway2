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

#include <interfaces/IPrivacy.h>
#include <interfaces/json/JsonData_Privacy.h>
#include <interfaces/json/JPrivacy.h>
#include "utils.h"


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
        class Privacy : public PluginHost::IPlugin, public PluginHost::JSONRPC {
        private:
            // We do not allow this plugin to be copied !!
            Privacy(const Privacy&) = delete;
            Privacy& operator=(const Privacy&) = delete;

        public:
            Privacy();
            virtual ~Privacy();
            virtual const string Initialize(PluginHost::IShell* shell) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override { return {}; }

            BEGIN_INTERFACE_MAP(Privacy)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IPrivacy, mPrivacy)
            END_INTERFACE_MAP

        private:
            class Notification : public RPC::IRemoteConnection::INotification,
                                 public Exchange::IPrivacy::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification &) = delete;
                Notification &operator=(const Notification &) = delete;

            public:
                explicit Notification(Privacy *parent) : mParent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification() override
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                INTERFACE_ENTRY(Exchange::IPrivacy::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection *) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                    mParent.Deactivated(connection);
                }

                void OnConsentStringChanged() override
                {
                    LOGINFO("OnConsentStringChanged\n");
                    Exchange::JPrivacy::Event::OnConsentStringChanged(mParent);
                }

                void OnContinueWatchingChanged(const bool allowed) override
                {
                    LOGINFO("OnContinueWatchingChanged\n");
                    Exchange::JPrivacy::Event::OnContinueWatchingChanged(mParent, allowed);
                }

                void OnPersonalizationChanged(const bool allowed) override
                {
                    LOGINFO("OnPersonalizationChanged\n");
                    Exchange::JPrivacy::Event::OnPersonalizationChanged(mParent, allowed);
                }

                void OnProductAnalyticsChanged(const bool allowed) override
                {
                    LOGINFO("OnProductAnalyticsChanged\n");
                    Exchange::JPrivacy::Event::OnProductAnalyticsChanged(mParent, allowed);
                }

                void OnWatchHistoryChanged(const bool allowed) override
                {
                    LOGINFO("OnWatchHistoryChanged\n");
                    Exchange::JPrivacy::Event::OnWatchHistoryChanged(mParent, allowed);
                }

                void OnACRCollectionChanged(const bool allowed) override
                {
                    LOGINFO("OnACRCollectionChanged\n");
                    Exchange::JPrivacy::Event::OnACRCollectionChanged(mParent, allowed);
                }

                void OnAppContentAdTargetingChanged(const bool allowed) override
                {
                    LOGINFO("OnAppContentAdTargetingChanged\n");
                    Exchange::JPrivacy::Event::OnAppContentAdTargetingChanged(mParent, allowed);
                }

                void OnCameraAnalyticsChanged(const bool allowed) override
                {
                    LOGINFO("OnCameraAnalyticsChanged\n");
                    Exchange::JPrivacy::Event::OnCameraAnalyticsChanged(mParent, allowed);
                }

                void OnPrimaryBrowseAdTargetingChanged(const bool allowed) override
                {
                    LOGINFO("OnPrimaryBrowseAdTargetingChanged\n");
                    Exchange::JPrivacy::Event::OnPrimaryBrowseAdTargetingChanged(mParent, allowed);
                }

                void OnPrimaryContentAdTargetingChanged(const bool allowed) override
                {
                    LOGINFO("OnPrimaryContentAdTargetingChanged\n");
                    Exchange::JPrivacy::Event::OnPrimaryContentAdTargetingChanged(mParent, allowed);
                }

                void OnRemoteDiagnosticsChanged(const bool allowed) override
                {
                    LOGINFO("OnRemoteDiagnosticsChanged\n");
                    Exchange::JPrivacy::Event::OnRemoteDiagnosticsChanged(mParent, allowed);
                }

                void OnUnentitledPersonalizationChanged(const bool allowed) override
                {
                    LOGINFO("OnUnentitledPersonalizationChanged\n");
                    Exchange::JPrivacy::Event::OnUnentitledPersonalizationChanged(mParent, allowed);
                }

                void OnUnentitledResumePointsChanged(const bool allowed) override
                {
                    LOGINFO("OnUnentitledResumePointsChanged\n");
                    Exchange::JPrivacy::Event::OnUnentitledResumePointsChanged(mParent, allowed);
                }

                void OnExclusionPolicyChanged() override
                {
                    LOGINFO("OnExclusionPolicyChanged\n");
                    Exchange::JPrivacy::Event::OnExclusionPolicyChanged(mParent);
                }

            private:
                Privacy &mParent;
            };

            void Deactivated(RPC::IRemoteConnection* connection);

        private:
            PluginHost::IShell* mService;
            uint32_t mConnectionId;
            Exchange::IPrivacy* mPrivacy;
            Core::Sink<Notification> mNotification;
        };
	} // namespace Plugin
} // namespace WPEFramework
