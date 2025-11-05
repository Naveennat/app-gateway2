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
#include "UtilsLogging.h"
#include "BaseEventDelegate.h"
#include <interfaces/IPrivacy.h>
#include <interfaces/IFbPrivacy.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IAppNotifications.h>
#include <memory>
#include <mutex>

namespace WPEFramework {
namespace Plugin {
    class FbPrivacyImplementation : public Exchange::IFbPrivacy, public Exchange::IConfiguration, public Exchange::IAppNotificationHandler {
    private:
        FbPrivacyImplementation(const FbPrivacyImplementation&) = delete;
        FbPrivacyImplementation& operator=(const FbPrivacyImplementation&) = delete;

    public:
        FbPrivacyImplementation();
        ~FbPrivacyImplementation();

        BEGIN_INTERFACE_MAP(FbPrivacyImplementation)
        INTERFACE_ENTRY(Exchange::IFbPrivacy)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        INTERFACE_ENTRY(Exchange::IAppNotificationHandler)
        END_INTERFACE_MAP

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);

        // IFbPrivacy interface
        Core::hresult Register(Exchange::IFbPrivacy::INotification* notification) override;
        Core::hresult Unregister(Exchange::IFbPrivacy::INotification* notification) override;
        Core::hresult AllowACRCollection(bool& value) const override;
        Core::hresult AllowAppContentAdTargeting(bool& value) const override;
        Core::hresult AllowCameraAnalytics(bool& value) const override;
        Core::hresult AllowPersonalization(bool& value) const override;
        Core::hresult AllowPrimaryBrowseAdTargeting(bool& value) const override;
        Core::hresult AllowPrimaryContentAdTargeting(bool& value) const override;
        Core::hresult AllowProductAnalytics(bool& value) const override;
        Core::hresult AllowRemoteDiagnostics(bool& value) const override;
        Core::hresult AllowResumePoints(bool& value) const override;
        Core::hresult AllowUnentitledPersonalization(bool& value) const override;
        Core::hresult AllowUnentitledResumePoints(bool& value) const override;
        Core::hresult AllowWatchHistory(bool& value) const override;
        Core::hresult Settings(Exchange::IFbPrivacy::PrivacySettings& out) const override;
        Core::hresult SetAllowACRCollection(const bool value) override;
        Core::hresult SetAllowAppContentAdTargeting(const bool value) override;
        Core::hresult SetAllowCameraAnalytics(const bool value) override;
        Core::hresult SetAllowPersonalization(const bool value) override;
        Core::hresult SetAllowPrimaryBrowseAdTargeting(const bool value) override;
        Core::hresult SetAllowPrimaryContentAdTargeting(const bool value) override;
        Core::hresult SetAllowProductAnalytics(const bool value) override;
        Core::hresult SetAllowRemoteDiagnostics(const bool value) override;
        Core::hresult SetAllowResumePoints(const bool value) override;
        Core::hresult SetAllowUnentitledPersonalization(const bool value) override;
        Core::hresult SetAllowUnentitledResumePoints(const bool value) override;
        Core::hresult SetAllowWatchHistory(const bool value) override;

        Core::hresult HandleAppEventNotifier(const string& event /* @in */, const bool& listen /* @in */, bool& status /* @out */) override;

    private:

        enum Setting
        {
            ALLOW_ACR_COLLECTION,
            ALLOW_APP_CONTENT_AD_TARGETING,
            ALLOW_CAMERA_ANALYTICS,
            ALLOW_PERSONALIZATION,
            ALLOW_PRIMARY_BROWSE_AD_TARGETING,
            ALLOW_PRIMARY_CONTENT_AD_TARGETING,
            ALLOW_PRODUCT_ANALYTICS,
            ALLOW_REMOTE_DIAGNOSTICS,
            ALLOW_RESUME_POINTS,
            ALLOW_UNENTITLED_PERSONALIZATION,
            ALLOW_UNENTITLED_RESUME_POINTS,
            ALLOW_WATCH_HISTORY
        };

        class EXTERNAL Job : public Core::IDispatch
        {
        protected:
            Job(FbPrivacyImplementation *parent, Setting setting, bool allowed)
                : mParent(*parent), mSetting(setting), mAllowed(allowed)
            {

            }

        public:
            Job() = delete;
            Job(const Job &) = delete;
            Job &operator=(const Job &) = delete;
            ~Job()
            {

            }

        public:
            static Core::ProxyType<Core::IDispatch> Create(FbPrivacyImplementation *parent, Setting setting, bool allowed)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(parent, setting, allowed)));
            }
            virtual void Dispatch()
            {
                mParent.Dispatch(mSetting, mAllowed);
            }

        private:
            FbPrivacyImplementation &mParent;
            const Setting mSetting;
            const bool mAllowed;
        };

        class PrivacyNotification : public Exchange::IPrivacy::INotification
        {
        private:
            PrivacyNotification() = delete;
            PrivacyNotification(const PrivacyNotification &) = delete;
            PrivacyNotification &operator=(const PrivacyNotification &) = delete;

        public:
            explicit PrivacyNotification(FbPrivacyImplementation *parent) : mParent(*parent)
            {
                ASSERT(parent != nullptr);
            }

            virtual ~PrivacyNotification() override
            {
            }

            BEGIN_INTERFACE_MAP(PrivacyNotification)
            INTERFACE_ENTRY(Exchange::IPrivacy::INotification)
            END_INTERFACE_MAP

            void OnConsentStringChanged()
            {
                // not used
            }

            void OnContinueWatchingChanged(const bool allowed)
            {
                LOGINFO("OnContinueWatchingChanged\n");

                mParent.DispatchEvent(ALLOW_RESUME_POINTS, allowed);
            }

            void OnPersonalizedRecommendationChanged(const bool allowed)
            {
                // not used
            }

            void OnProductAnalyticsChanged(const bool allowed)
            {
                LOGINFO("OnProductAnalyticsChanged\n");
                mParent.DispatchEvent(ALLOW_PRODUCT_ANALYTICS, allowed);
            }

            void OnWatchHistoryChanged(const bool allowed)
            {
                LOGINFO("OnWatchHistoryChanged\n");
                mParent.DispatchEvent(ALLOW_WATCH_HISTORY, allowed);
            }

            void OnACRCollectionChanged(const bool value) override
            {
                LOGINFO("OnAllowACRCollectionChanged\n");
                mParent.DispatchEvent(ALLOW_ACR_COLLECTION, value);
            }

            void OnAppContentAdTargetingChanged(const bool allowed)
            {
                LOGINFO("OnAppContentAdTargetingChanged\n");
                mParent.DispatchEvent(ALLOW_APP_CONTENT_AD_TARGETING, allowed);
            }

            void OnCameraAnalyticsChanged(const bool allowed)
            {
                LOGINFO("OnCameraAnalyticsChanged\n");
                mParent.DispatchEvent(ALLOW_CAMERA_ANALYTICS, allowed);
            }

            void OnPersonalizationChanged(const bool allowed)
            {
                LOGINFO("OnPersonalizationChanged\n");
                mParent.DispatchEvent(ALLOW_PERSONALIZATION, allowed);
            }

            void OnPrimaryBrowseAdTargetingChanged(const bool allowed)
            {
                LOGINFO("OnPrimaryBrowseAdTargetingChanged\n");
                mParent.DispatchEvent(ALLOW_PRIMARY_BROWSE_AD_TARGETING, allowed);
            }

            void OnPrimaryContentAdTargetingChanged(const bool allowed)
            {
                LOGINFO("OnPrimaryContentAdTargetingChanged\n");
                mParent.DispatchEvent(ALLOW_PRIMARY_CONTENT_AD_TARGETING, allowed);
            }

            void OnRemoteDiagnosticsChanged(const bool allowed)
            {
                LOGINFO("OnRemoteDiagnosticsChanged\n");
                mParent.DispatchEvent(ALLOW_REMOTE_DIAGNOSTICS, allowed);
            }

            void OnUnentitledPersonalizationChanged(const bool allowed)
            {
                LOGINFO("OnUnentitledPersonalizationChanged\n");
                mParent.DispatchEvent(ALLOW_UNENTITLED_PERSONALIZATION, allowed);
            }

            void OnUnentitledResumePointsChanged(const bool allowed)
            {
                LOGINFO("OnUnentitledResumePointsChanged\n");
                mParent.DispatchEvent(ALLOW_UNENTITLED_RESUME_POINTS, allowed);
            }

            void OnExclusionPolicyChanged()
            {
                // not used
            }

        private:
            FbPrivacyImplementation &mParent;
        };

        void DispatchEvent(Setting setting, bool allowed);
        void Dispatch(Setting setting, bool allowed);
        uint32_t Set(Setting setting, bool allowed);
        bool Get(Setting setting) const;

        PluginHost::IShell* mShell;
        std::mutex mNotificationMutex;
        std::list<Exchange::IFbPrivacy::INotification*> mNotifications;
        Core::Sink<PrivacyNotification> mPrivacyNotification;
        std::shared_ptr<BaseEventDelegate> mAppNotificationDelegate;
    };
}
}
