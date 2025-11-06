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

#include "../Module.h"
#include <interfaces/IPrivacy.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IStore.h>
#include <interfaces/IFbAsLinchPin.h>
#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>

#include "EssClient.h"
#include "DeviceInfo.h"

#include <mutex>
#include <condition_variable>
#include <thread>
#include <queue>

namespace WPEFramework {
namespace Plugin {
    class PrivacyImplementation : public Exchange::IPrivacy, public Exchange::IConfiguration {
    private:
        PrivacyImplementation(const PrivacyImplementation&) = delete;
        PrivacyImplementation& operator=(const PrivacyImplementation&) = delete;

    public:
        PrivacyImplementation();
        ~PrivacyImplementation();

        BEGIN_INTERFACE_MAP(PrivacyImplementation)
        INTERFACE_ENTRY(Exchange::IPrivacy)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

    private:

        enum ActionType {
            ACTION_TYPE_INIT,
            ACTION_TYPE_SYNC_CONSENT_STRINGS,
            ACTION_TYPE_SYNC_EXCLUSION_POLICIES,
            ACTION_TYPE_SYNC_PRIVACY_SETTINGS,
            ACTION_TYPE_PROCESS_CONSENT_STRING_CHANGED,
            ACTION_TYPE_PROCESS_EXCLUSION_POLICY_CHANGED,
            ACTION_TYPE_PROCESS_PRIVACY_SETTINGS_CHANGED,
            ACTION_TYPE_SHUTDOWN,
            ACTION_TYPE_NO_ACTION
        };

        struct Action {
            ActionType type;
        };

        enum Event
        {
            CONSENT_STRING_CHANGED,
            EXCLUSION_POLICY_CHANGED,
            SETTING_CONTINUE_WATCHING_CHANGED,
            SETTING_PRODUCT_ANALYTICS_CHANGED,
            SETTING_WATCH_HISTORY_CHANGED,
            SETTING_ACR_COLLECTION_CHANGED,
            SETTING_APP_CONTENT_AD_TARGETING_CHANGED,
            SETTING_CAMERA_ANALYTICS_CHANGED,
            SETTING_PERSONALIZATION_CHANGED,
            SETTING_PRIMARY_BROWSE_AD_TARGETING_CHANGED,
            SETTING_PRIMARY_CONTENT_AD_TARGETING_CHANGED,
            SETTING_REMOTE_DIAGNOSTICS_CHANGED,
            SETTING_UNENTITLED_PERSONALIZATION_CHANGED,
            SETTING_UNENTITLED_RESUME_POINTS_CHANGED
        };

        class EXTERNAL Job : public Core::IDispatch
        {
        protected:
            Job(PrivacyImplementation *parent, Event event, bool allowed)
                : mParent(*parent), mEvent(event), mAllowed(allowed)
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
            static Core::ProxyType<Core::IDispatch> Create(PrivacyImplementation *parent, Event event, bool allowed)
            {
#ifndef USE_THUNDER_R4
                return (Core::proxy_cast<Core::IDispatch>(Core::ProxyType<Job>::Create(parent, event, allowed)));
#else
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<Job>::Create(parent, event, allowed)));
#endif
            }
            virtual void Dispatch()
            {
                mParent.Dispatch(mEvent, mAllowed);
            }

        private:
            PrivacyImplementation &mParent;
            const Event mEvent;
            const bool mAllowed;
        };

        class Timer
        {
        public:
            Timer() : mRunning(false) {}

            // Start the timer and call the handler after `seconds`
            void start(int seconds, std::function<void()> handler)
            {
                stop(); // Ensure no previous timer is running

                mRunning = true;
                mHandler = std::move(handler);
                mTimerThread = std::move(std::thread([this, seconds]() mutable
                    {
                        std::unique_lock<std::mutex> lock(mTimerMutex);
                        if (!mCv.wait_until(lock, std::chrono::system_clock::now() 
                            + std::chrono::seconds(seconds), [this] { return !mRunning; })) {
                            mHandler(); // Execute handler if timer wasn't stopped
                    } }));
            }

            // Stop the timer before execution
            void stop()
            {
                {
                    std::lock_guard<std::mutex> lock(mTimerMutex);
                    mRunning = false;
                }
                mCv.notify_all();
                if (mTimerThread.joinable())
                {
                    mTimerThread.join();
                }
            }

            ~Timer()
            {
                stop();
            }

        private:
            std::thread mTimerThread;
            std::atomic<bool> mRunning;
            std::mutex mTimerMutex;
            std::function<void()> mHandler;
            std::condition_variable mCv;
        };

        class MonitorKeys : public Exchange::IStore::INotification
        {
        private:
            MonitorKeys(const MonitorKeys &) = delete;
            MonitorKeys &operator=(const MonitorKeys &) = delete;

        public:
            MonitorKeys() : mCallbacks() {}
            ~MonitorKeys() = default;

            typedef std::function<void(const std::string &)> Callback;

            void ValueChanged(const string &ns, const string &key, const string &value) override;
            void StorageExceeded() override;

            void RegisterCallback(const string &ns, const string &key, Callback callback);

            BEGIN_INTERFACE_MAP(MonitorKeys)
            INTERFACE_ENTRY(Exchange::IStore::INotification)
            END_INTERFACE_MAP
        private:
            std::map<std::string, std::map<std::string, Callback>> mCallbacks;
        };

        enum class LinchPinNotificationType {
            EVENT,
            CONNECTED,
            DISCONNECTED
        };

        class LinchPinNotification : public Exchange::IFbAsLinchPin::INotification
        {
        private:
            LinchPinNotification(const LinchPinNotification &) = delete;
            LinchPinNotification &operator=(const LinchPinNotification &) = delete;

        public:
            LinchPinNotification(PrivacyImplementation* parent) : mParent(*parent) {}
            ~LinchPinNotification() override = default;
            void OnEvent(const string &value /* @in @opaque */) override
            {
                mParent.LinchPinNotificationReceived(LinchPinNotificationType::EVENT, value);
            }
            void OnConnected() override
            {
                mParent.LinchPinNotificationReceived(LinchPinNotificationType::CONNECTED, "");
            }
            void OnDisconnected() override
            {
                mParent.LinchPinNotificationReceived(LinchPinNotificationType::DISCONNECTED, "");
            }

            BEGIN_INTERFACE_MAP(LinchPinNotification)
            INTERFACE_ENTRY(Exchange::IFbAsLinchPin::INotification)
            END_INTERFACE_MAP
        private:
            PrivacyImplementation &mParent;
        };

        // IPrivacyImplementation interface
        Core::hresult Register(Exchange::IPrivacy::INotification *notification) override;
        Core::hresult Unregister(Exchange::IPrivacy::INotification *notification) override;

        Core::hresult GetConsentString(const string &adUsecase, string &gppValue, string &gppSid, string &decoded) const override;
        Core::hresult GetContinueWatching(Exchange::IPrivacy::PrivacySettingOutData &data) const override;
        Core::hresult GetPersonalization(Exchange::IPrivacy::PrivacySettingOutData &data) const override;
        Core::hresult GetProductAnalytics(Exchange::IPrivacy::PrivacySettingOutData &data) const override;
        Core::hresult GetWatchHistory(Exchange::IPrivacy::PrivacySettingOutData &data) const override;
        Core::hresult GetACRCollection(PrivacySettingOutData &data) const override;
        Core::hresult GetAppContentAdTargeting(PrivacySettingOutData &data) const override;
        Core::hresult GetCameraAnalytics(PrivacySettingOutData &data) const override;
        Core::hresult GetPrimaryBrowseAdTargeting(PrivacySettingOutData &data) const override;
        Core::hresult GetPrimaryContentAdTargeting(PrivacySettingOutData &data) const override;
        Core::hresult GetRemoteDiagnostics(PrivacySettingOutData &data) const override;
        Core::hresult GetUnentitledPersonalization(PrivacySettingOutData &data) const override;
        Core::hresult GetUnentitledResumePoints(PrivacySettingOutData &data) const override;
        Core::hresult SetContinueWatching(const bool allowed) override;
        Core::hresult SetPersonalization(const bool allowed) override;
        Core::hresult SetProductAnalytics(const bool allowed) override;
        Core::hresult SetWatchHistory(const bool allowed) override;
        Core::hresult SetACRCollection(const bool allowed) override;
        Core::hresult SetAppContentAdTargeting(const bool allowed) override;
        Core::hresult SetCameraAnalytics(const bool allowed) override;
        Core::hresult SetPrimaryBrowseAdTargeting(const bool allowed) override;
        Core::hresult SetPrimaryContentAdTargeting(const bool allowed) override;
        Core::hresult SetRemoteDiagnostics(const bool allowed) override;
        Core::hresult SetUnentitledPersonalization(const bool allowed) override;
        Core::hresult SetUnentitledResumePoints(const bool allowed) override;
        Core::hresult GetAllPrivacySettings(Exchange::IPrivacy::IPrivacySettingInfoIterator* &settings) const override;
        Core::hresult GetExclusionPolicies(Exchange::IPrivacy::IStringIterator* &policies) const override;
        Core::hresult GetDataForExclusionPolicy(const string& policy, IStringIterator* &dataEvents, IStringIterator* &entityReference, bool &derivativePropagation) const override;

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);

        void ActionLoop();
        Core::hresult GetPrivacySetting(const string &settingName, bool &allowed) const;
        Core::hresult SetPrivacySetting(const string &settingName, bool allowed);
        void Init();
        void SyncConsentStrings();
        uint32_t GetConsentStringsFromCache(std::unordered_map<string, EssConsentString> &consentStringsMap);
        uint32_t SetConsentStringsCache(const std::unordered_map<string, EssConsentString> &consentStringsMap);
        uint32_t ParseCachedConsentStrings(const string &cache, std::unordered_map<string, EssConsentString> &consentStringsMap);
        void SyncExclusionPolicies();
        uint32_t GetExclusionPoliciesFromCache(EssExclusionPolicies &policies, uint64_t &lastSyncTime);
        uint32_t SetExclusionPoliciesCache(const EssExclusionPolicies &policies, const uint64_t &lastSyncTime);
        uint32_t ParseCachedExclusionPolicies(const string &cache, EssExclusionPolicies &policies, uint64_t &lastSyncTime);
        uint64_t GetCurrentTimestampInMs() const;
        void SyncPrivacySettings();
        void PrivacySettingsEmitEventIfNeeded(const EssPrivacySettings &pOld, const EssPrivacySettings &pNew);

        void DispatchEvent(Event event, bool allowed);
        void Dispatch(Event event, bool allowed);

        void LinchPinNotificationReceived(LinchPinNotificationType type, const string &value);

        mutable std::mutex mQueueMutex;
        mutable std::condition_variable mQueueCondition;
        std::thread mThread;
        mutable std::queue<Action> mActionQueue;

        PluginHost::IShell* mShell;

        std::mutex mNotificationMutex;
        std::list<Exchange::IPrivacy::INotification*> mNotifications;
        DeviceInfoPtr mDeviceInfoPtr;
        EssClientPtr mEssClientPtr;
        uint32_t mEssBaseRetryBackoffTimeSec;
        uint32_t mEssRetryLimit;

        mutable std::mutex mConsentStringsMutex;
        mutable std::unordered_map<string, EssConsentString> mConsentStringsMap;
        Timer mConsentStringsSyncTimer;
        Timer mConsentStringsExpiredTimer;
        uint32_t mConsentStringsRetryBackoffTimeSec;
        uint32_t mConsentStringsRetryLimit;

        mutable std::mutex mExclusionPoliciesMutex;
        mutable EssExclusionPolicies mExclusionPolicies;
        Timer mExclusionPoliciesSyncTimer;
        uint32_t mExclusionPoliciesBackoffTimeSec;
        uint32_t mExclusionPoliciesRetryLimit;
        uint32_t mEssExclusionPoliciesSyncSec;
        mutable uint64_t mLastExclusionPoliciesSuccesfullSyncTime;

        mutable std::mutex mPrivacySettingsMutex;
        mutable EssPrivacySettings mPrivacySettings;
        bool mPrivacySettingsCacheValid;
        bool mPrivacySettingsLinchPinEventsEnabled;
        Timer mPrivacySettingsSyncTimer;
        uint32_t mPrivacySettingsBackoffTimeSec;
        uint32_t mPrivacySettingsRetryLimit;

        bool mInitialized;
        Core::Sink<MonitorKeys> mMonitorCache;
        Core::Sink<LinchPinNotification> mLinchPinNotificationSink;
    };
}
}
