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
#include "PrivacyImplementation.h"
#include "UtilsTelemetry.h"
#include "utils.h"
#include "UtilsCallsign.h"

#include <interfaces/IStore.h>

#include <fstream>
#include <streambuf>
#include <sys/sysinfo.h>


#define PRIVACY_INIT_RETRY_MS 3000
#define PRIVACY_CONSENT_STRINGS_SYNC_INTERVAL_SEC (60 * 60 * 24) // 24 hours

#define PERSISTENT_STORE_CALLSIGN "org.rdk.PersistentStore"
#define PERSISTENT_STORE_PRIVACY_NAMESPACE "Privacy"
#define PERSISTENT_STORE_PRIVACY_CONSET_STRINGS_KEY "ConsentStrings"
#define PERSISTENT_STORE_PRIVACY_EXCLUSION_POLICIES_KEY "ExclusionPolicies"

namespace WPEFramework {
namespace Plugin {


    class PrivacyConfig : public Core::JSON::Container {
        private:
            PrivacyConfig(const PrivacyConfig&) = delete;
            PrivacyConfig& operator=(const PrivacyConfig&) = delete;

        public:
            PrivacyConfig()
                : Core::JSON::Container()
                , EssUrl()
                , ClientId()
                , EssSettingsBaseRetryBackoffTimeSec()
                , EssSettingsRetryLimit()
                , EssExclusionPolicySyncSec(43200) // 12 hours default
            {
                Add(_T("essurl"), &EssUrl);
                Add(_T("clientid"), &ClientId);
                Add(_T("esssettingsbaseretrybackofftimesec"), &EssSettingsBaseRetryBackoffTimeSec);
                Add(_T("esssettingsretrylimit"), &EssSettingsRetryLimit);
                Add(_T("essexclusionpolicysyncsec"), &EssExclusionPolicySyncSec);
            }
            ~PrivacyConfig()
            {
            }

        public:
            Core::JSON::String EssUrl;
            Core::JSON::String ClientId;
            Core::JSON::DecUInt32 EssSettingsBaseRetryBackoffTimeSec;
            Core::JSON::DecUInt32 EssSettingsRetryLimit;
            Core::JSON::DecUInt32 EssExclusionPolicySyncSec;
        };

    SERVICE_REGISTRATION(PrivacyImplementation, 1, 0);

    PrivacyImplementation::PrivacyImplementation():
        mQueueMutex(),
        mQueueCondition(),
        mActionQueue(),
        mShell(nullptr),
        mNotificationMutex(),
        mNotifications(),
        mDeviceInfoPtr(nullptr),
        mEssClientPtr(nullptr),
        mEssBaseRetryBackoffTimeSec(0),
        mEssRetryLimit(0),
        mConsentStringsMutex(),
        mConsentStringsMap(),
        mConsentStringsSyncTimer(),
        mConsentStringsExpiredTimer(),
        mConsentStringsRetryBackoffTimeSec(0),
        mConsentStringsRetryLimit(0),
        mExclusionPoliciesMutex(),
        mExclusionPolicies(),
        mExclusionPoliciesSyncTimer(),
        mExclusionPoliciesBackoffTimeSec(0),
        mExclusionPoliciesRetryLimit(0),
        mEssExclusionPoliciesSyncSec(0),
        mLastExclusionPoliciesSuccesfullSyncTime(0),
        mPrivacySettingsMutex(),
        mPrivacySettings(),
        mPrivacySettingsCacheValid(false),
        mPrivacySettingsLinchPinEventsEnabled(false),
        mPrivacySettingsSyncTimer(),
        mPrivacySettingsBackoffTimeSec(0),
        mPrivacySettingsRetryLimit(0),
        mInitialized(false),
        mMonitorCache(),
        mLinchPinNotificationSink(this)
    {
        LOGINFO("PrivacyImplementation::PrivacyImplementation()");
        Utils::Telemetry::Init();

        mMonitorCache.RegisterCallback(
            PERSISTENT_STORE_PRIVACY_NAMESPACE,
            PERSISTENT_STORE_PRIVACY_CONSET_STRINGS_KEY,
            [this](const string &value)
            {
                LOGINFO("Consent strings cache changed");
                std::unique_lock<std::mutex> lock(mConsentStringsMutex);
                mConsentStringsMap.clear();
                ParseCachedConsentStrings(value, mConsentStringsMap);
            });

        mMonitorCache.RegisterCallback(
            PERSISTENT_STORE_PRIVACY_NAMESPACE,
            PERSISTENT_STORE_PRIVACY_EXCLUSION_POLICIES_KEY,
            [this](const string &value)
            {
                LOGINFO("Exclusion policies cache changed");
                std::unique_lock<std::mutex> lock(mExclusionPoliciesMutex);
                mExclusionPolicies = {};
                ParseCachedExclusionPolicies(value, mExclusionPolicies, mLastExclusionPoliciesSuccesfullSyncTime);
            });
    }

    PrivacyImplementation::~PrivacyImplementation()
    {
        LOGINFO("PrivacyImplementation::~PrivacyImplementation()");
        std::unique_lock<std::mutex> lock(mQueueMutex);
        Action action = {ACTION_TYPE_SHUTDOWN};
        mActionQueue.push(action);
        lock.unlock();
        mQueueCondition.notify_one();
        if (mThread.joinable())
        {
            mThread.join();
        }

        auto interface = mShell->QueryInterfaceByCallsign<Exchange::IStore>(PERSISTENT_STORE_CALLSIGN);
        if (interface == nullptr)
        {
            LOGERR("No IStore");
        }
        else
        {
            uint32_t result = interface->Unregister(&mMonitorCache);
            LOGINFO("IStore status %d", result);
            interface->Release();
        }

        auto interfaceLinchPin = mShell->QueryInterfaceByCallsign<Exchange::IFbAsLinchPin>(FB_AS_LINCHPIN_CALLSIGN);
        if (interfaceLinchPin)
        {
            interfaceLinchPin->Unregister(&mLinchPinNotificationSink);
            interfaceLinchPin->Release();
        }

        mShell->Release();
    }

    uint32_t PrivacyImplementation::Configure(PluginHost::IShell* shell)
    {
        LOGINFO("Configuring Privacy");
        uint32_t result = Core::ERROR_NONE;
        ASSERT(shell != nullptr);
        mShell = shell;
        mShell->AddRef();

        std::string configLine = mShell->ConfigLine();
        Core::OptionalType<Core::JSON::Error> error;
        PrivacyConfig config;

        if (config.FromString(configLine, error) == false)
        {
            SYSLOG(Logging::ParsingError,
                   (_T("Failed to parse config line, error: '%s', config line: '%s'."),
                    (error.IsSet() ? error.Value().Message().c_str() : "Unknown"),
                    configLine.c_str()));
        }

        mEssBaseRetryBackoffTimeSec = config.EssSettingsBaseRetryBackoffTimeSec.Value();
        mEssRetryLimit = config.EssSettingsRetryLimit.Value();
        mConsentStringsRetryBackoffTimeSec = mEssBaseRetryBackoffTimeSec;
        mConsentStringsRetryLimit = mEssRetryLimit;

        mPrivacySettingsBackoffTimeSec = mEssBaseRetryBackoffTimeSec;
        mPrivacySettingsRetryLimit = mEssRetryLimit;

        mEssExclusionPoliciesSyncSec = config.EssExclusionPolicySyncSec.Value();

        mDeviceInfoPtr = std::make_shared<DeviceInfo>(shell);
        if (mDeviceInfoPtr == nullptr)
        {
            LOGERR("Failed to create DeviceInfo");
            return Core::ERROR_GENERAL;
        }

        mEssClientPtr = std::make_shared<EssClient>(config.EssUrl.Value(), config.ClientId.Value(), mDeviceInfoPtr);
        if (mEssClientPtr == nullptr)
        {
            LOGERR("Failed to create EssClient");
            return Core::ERROR_GENERAL;
        }

        auto interfaceLinchPin = mShell->QueryInterfaceByCallsign<Exchange::IFbAsLinchPin>(FB_AS_LINCHPIN_CALLSIGN);
        if (interfaceLinchPin)
        {
            interfaceLinchPin->Register(&mLinchPinNotificationSink);
            interfaceLinchPin->Release();
            LOGINFO("Registered for LinchPin notifications");
        }

        mThread = std::thread(&PrivacyImplementation::ActionLoop, this);

        return result;
    }

    Core::hresult PrivacyImplementation::Register(Exchange::IPrivacy::INotification *notification)
    {
        ASSERT (nullptr != notification);
        std::unique_lock<std::mutex> lock(mNotificationMutex);
        // Make sure we can't register the same notification callback multiple times
        if (std::find(mNotifications.begin(), mNotifications.end(), notification) == mNotifications.end())
        {
            LOGINFO("Register notification");
            mNotifications.push_back(notification);
            notification->AddRef();
        }
        return Core::ERROR_NONE;
    }

    Core::hresult PrivacyImplementation::Unregister(Exchange::IPrivacy::INotification *notification)
    {
        Core::hresult status = Core::ERROR_GENERAL;
        ASSERT (nullptr != notification);
        std::unique_lock<std::mutex> lock(mNotificationMutex);
        // Make sure we can't unregister the same notification callback multiple times
        auto itr = std::find(mNotifications.begin(), mNotifications.end(), notification);
        if (itr != mNotifications.end())
        {
            (*itr)->Release();
            LOGINFO("Unregister notification");
            mNotifications.erase(itr);
            status = Core::ERROR_NONE;
        }
        else
        {
            LOGERR("notification not found");
        }
        return status;
    }

    Core::hresult PrivacyImplementation::GetConsentString(const string &adUsecase, string &gppValue, string &gppSid, string &decoded) const
    {
        LOGINFO("GetConsentString");
        EssConsentString consentString;
        uint32_t err = mEssClientPtr->GetConsentString(adUsecase, consentString);
        if (err != Core::ERROR_NONE)
        {
            LOGERR("Failed to get consent string from ESS: %d", err);
            Utils::Telemetry::SendError("Privacy: Failed to get consent string from ESS: %d", err);
            // TODO: return cached values once linchpin supporteed and caching rules defines
            if (err == Core::ERROR_GENERAL || err == Core::ERROR_NOT_SUPPORTED)
            {
                err = Core::ERROR_UNAVAILABLE;
            }
            return err;
        }

        gppValue = consentString.gppValue;
        gppSid = consentString.gppSid;
        decoded = consentString.decoded;

        // Compare with cached mConsentStringsMap
        bool hasChanged = false;
        std::unique_lock<std::mutex> lock(mConsentStringsMutex);
        if (mConsentStringsMap.find(adUsecase) == mConsentStringsMap.end())
        {
            mConsentStringsMap[adUsecase] = std::move(consentString);
            hasChanged = true;
        }
        else
        {
            if (mConsentStringsMap[adUsecase] != consentString)
            {
                mConsentStringsMap[adUsecase] = std::move(consentString);
                hasChanged = true;
            }
        }

        lock.unlock();

        if (hasChanged)
        {
            {
                std::unique_lock<std::mutex> lockQueue(mQueueMutex);
                Action action = {ACTION_TYPE_PROCESS_CONSENT_STRING_CHANGED};
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        }

        return Core::ERROR_NONE;
    }

    Core::hresult PrivacyImplementation::GetContinueWatching(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetContinueWatching");
        return GetPrivacySetting(EssPrivacySettingContinueWatching, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetPersonalization(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetPersonalization");
        return GetPrivacySetting(EssPrivacySettingPersonalization, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetProductAnalytics(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetProductAnalytics");
        return GetPrivacySetting(EssPrivacySettingProductAnalytics, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetWatchHistory(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetWatchHistory");
        return GetPrivacySetting(EssPrivacySettingWatchHistory, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetACRCollection(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetACRCollection");
        return GetPrivacySetting(EssPrivacySettingAcrCollection, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetAppContentAdTargeting(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetAppContentAdTargeting");
        return GetPrivacySetting(EssPrivacySettingAppContentAdTargeting, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetCameraAnalytics(Exchange::IPrivacy::PrivacySettingOutData &data) const
    {
        LOGINFO("GetCameraAnalytics");
        return GetPrivacySetting(EssPrivacySettingCameraAnalytics, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetPrimaryBrowseAdTargeting(PrivacySettingOutData &data) const
    {
        LOGINFO("GetPrimaryBrowseAdTargeting");
        return GetPrivacySetting(EssPrivacySettingPrimaryBrowseAdTargeting, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetPrimaryContentAdTargeting(PrivacySettingOutData &data) const
    {
        LOGINFO("GetPrimaryContentAdTargeting");
        return GetPrivacySetting(EssPrivacySettingPrimaryContentAdTargeting, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetRemoteDiagnostics(PrivacySettingOutData &data) const
    {
        LOGINFO("GetRemoteDiagnostics");
        return GetPrivacySetting(EssPrivacySettingRemoteDiagnostics, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetUnentitledPersonalization(PrivacySettingOutData &data) const
    {
        LOGINFO("GetUnentitledPersonalization");
        return GetPrivacySetting(EssPrivacySettingUnentitledPersonalization, data.allowed);
    }

    Core::hresult PrivacyImplementation::GetUnentitledResumePoints(PrivacySettingOutData &data) const
    {
        LOGINFO("GetUnentitledResumePoints");
        return GetPrivacySetting(EssPrivacySettingUnentitledContinueWatching, data.allowed);
    }

    Core::hresult PrivacyImplementation::SetContinueWatching(const bool allowed)
    {
        LOGINFO("SetContinueWatching");
        return SetPrivacySetting(EssPrivacySettingContinueWatching, allowed);
    }

    Core::hresult PrivacyImplementation::SetPersonalization(const bool allowed)
    {
        LOGINFO("SetPersonalization");
        return SetPrivacySetting(EssPrivacySettingPersonalization, allowed);
    }

    Core::hresult PrivacyImplementation::SetProductAnalytics(const bool allowed)
    {
        LOGINFO("SetProductAnalytics");
        return SetPrivacySetting(EssPrivacySettingProductAnalytics, allowed);
    }

    Core::hresult PrivacyImplementation::SetWatchHistory(const bool allowed)
    {
        LOGINFO("SetWatchHistory");
        return SetPrivacySetting(EssPrivacySettingWatchHistory, allowed);
    }

    Core::hresult PrivacyImplementation::SetACRCollection(const bool allowed)
    {
        LOGINFO("SetACRCollection");
        return SetPrivacySetting(EssPrivacySettingAcrCollection, allowed);
    }

    Core::hresult PrivacyImplementation::SetAppContentAdTargeting(const bool allowed)
    {
        LOGINFO("SetAppContentAdTargeting");
        return SetPrivacySetting(EssPrivacySettingAppContentAdTargeting, allowed);
    }

    Core::hresult PrivacyImplementation::SetCameraAnalytics(const bool allowed)
    {
        LOGINFO("SetCameraAnalytics");
        return SetPrivacySetting(EssPrivacySettingCameraAnalytics, allowed);
    }

    Core::hresult PrivacyImplementation::SetPrimaryBrowseAdTargeting(const bool allowed)
    {
        LOGINFO("SetPrimaryBrowseAdTargeting");
        return SetPrivacySetting(EssPrivacySettingPrimaryBrowseAdTargeting, allowed);
    }

    Core::hresult PrivacyImplementation::SetPrimaryContentAdTargeting(const bool allowed)
    {
        LOGINFO("SetPrimaryContentAdTargeting");
        return SetPrivacySetting(EssPrivacySettingPrimaryContentAdTargeting, allowed);
    }

    Core::hresult PrivacyImplementation::SetRemoteDiagnostics(const bool allowed)
    {
        LOGINFO("SetRemoteDiagnostics");
        return SetPrivacySetting(EssPrivacySettingRemoteDiagnostics, allowed);
    }

    Core::hresult PrivacyImplementation::SetUnentitledPersonalization(const bool allowed)
    {
        LOGINFO("SetUnentitledPersonalization");
        return SetPrivacySetting(EssPrivacySettingUnentitledPersonalization, allowed);
    }

    Core::hresult PrivacyImplementation::SetUnentitledResumePoints(const bool allowed)
    {
        LOGINFO("SetUnentitledResumePoints");
        return SetPrivacySetting(EssPrivacySettingUnentitledContinueWatching, allowed);
    }

    Core::hresult PrivacyImplementation::GetPrivacySetting(const string &settingName, bool &allowed) const
    {
        {
            // check if cache is available
            std::unique_lock<std::mutex> lock(mPrivacySettingsMutex);
            if (mPrivacySettingsCacheValid)
            {
                LOGINFO("Using cached privacy settings");
                std::map<const string, const EssPrivacySettingData &> settingsMap =
                    {
                        {EssPrivacySettingContinueWatching, mPrivacySettings.continueWatching},
                        {EssPrivacySettingProductAnalytics, mPrivacySettings.productAnalytics},
                        {EssPrivacySettingWatchHistory, mPrivacySettings.watchHistory},
                        {EssPrivacySettingAcrCollection, mPrivacySettings.acrCollection},
                        {EssPrivacySettingAppContentAdTargeting, mPrivacySettings.appContentAdTargeting},
                        {EssPrivacySettingCameraAnalytics, mPrivacySettings.cameraAnalytics},
                        {EssPrivacySettingPersonalization, mPrivacySettings.personalization},
                        {EssPrivacySettingPrimaryBrowseAdTargeting, mPrivacySettings.primaryBrowseAdTargeting},
                        {EssPrivacySettingPrimaryContentAdTargeting, mPrivacySettings.primaryContentAdTargeting},
                        {EssPrivacySettingRemoteDiagnostics, mPrivacySettings.remoteDiagnostics},
                        {EssPrivacySettingUnentitledPersonalization, mPrivacySettings.unentitledPersonalization},
                        {EssPrivacySettingUnentitledContinueWatching, mPrivacySettings.unentitledContinueWatching}};

                if (settingsMap.find(settingName) == settingsMap.end() || settingsMap.at(settingName).available == false)
                {
                    LOGERR("Setting %s not available", settingName.c_str());
                    Utils::Telemetry::SendError("Privacy: Setting %s not available", settingName.c_str());
                    return Core::ERROR_UNAVAILABLE;
                }

                allowed = settingsMap.at(settingName).allowed;
                return Core::ERROR_NONE;
            }
        }

        EssPrivacySettingData privacySetting;
        uint32_t err = mEssClientPtr->GetPrivacySetting(settingName, privacySetting);
        if (err != Core::ERROR_NONE)
        {
            LOGERR("Failed to get privacy settings: %d", err);
            Utils::Telemetry::SendError("Privacy: Failed to get privacy setting %s: %d", settingName.c_str(), err);
            if (err == Core::ERROR_GENERAL || err == Core::ERROR_NOT_SUPPORTED)
            {
                err = Core::ERROR_UNAVAILABLE;
            }
            return err;
        }

        if (!privacySetting.available)
        {
            LOGERR("Privacy setting not available");
            Utils::Telemetry::SendError("Privacy: Privacy setting %s not available", settingName.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        allowed = privacySetting.allowed;

        return Core::ERROR_NONE;
    }

    Core::hresult PrivacyImplementation::SetPrivacySetting(const string &settingName, bool allowed)
    {
        EssPrivacySettings privacySettings;
        std::map<const string, EssPrivacySettingData &> settingsMap =
        {
            {EssPrivacySettingContinueWatching, privacySettings.continueWatching},
            {EssPrivacySettingProductAnalytics, privacySettings.productAnalytics},
            {EssPrivacySettingWatchHistory, privacySettings.watchHistory},
            {EssPrivacySettingAcrCollection, privacySettings.acrCollection},
            {EssPrivacySettingAppContentAdTargeting, privacySettings.appContentAdTargeting},
            {EssPrivacySettingCameraAnalytics, privacySettings.cameraAnalytics},
            {EssPrivacySettingPersonalization, privacySettings.personalization},
            {EssPrivacySettingPrimaryBrowseAdTargeting, privacySettings.primaryBrowseAdTargeting},
            {EssPrivacySettingPrimaryContentAdTargeting, privacySettings.primaryContentAdTargeting},
            {EssPrivacySettingRemoteDiagnostics, privacySettings.remoteDiagnostics},
            {EssPrivacySettingUnentitledPersonalization, privacySettings.unentitledPersonalization},
            {EssPrivacySettingUnentitledContinueWatching, privacySettings.unentitledContinueWatching}
        };

        if (settingsMap.find(settingName) == settingsMap.end())
        {
            LOGERR("Invalid setting name");
            Utils::Telemetry::SendError("Privacy: Invalid setting name %s", settingName.c_str());
            return Core::ERROR_NOT_SUPPORTED;
        }

        string accountId;
        uint32_t ret = mDeviceInfoPtr->GetAccountId(accountId);
        if (ret != Core::ERROR_NONE)
        {
            LOGERR("Failed to get accountId: %d", ret);
            return Core::ERROR_NOT_SUPPORTED;
        }

        settingsMap.at(settingName).available = true;
        settingsMap.at(settingName).allowed = allowed;
        //TODO: remove once caching of PrivacySettings is implemented
        settingsMap.at(settingName).ownerReference = "xrn:xcal:subscriber:account:" + accountId;

        ret = mEssClientPtr->SetPrivacySettings(privacySettings);
        if (ret != Core::ERROR_NONE)
        {
            LOGERR("Failed to set privacy setting %s: %d", settingName.c_str(), ret);
            Utils::Telemetry::SendError("Privacy: Failed to set privacy setting %s: %d", settingName.c_str(), ret);
            if (ret == Core::ERROR_GENERAL)
            {
                ret = Core::ERROR_NOT_SUPPORTED;
            }
        }

        return ret;
    }

    Core::hresult PrivacyImplementation::GetAllPrivacySettings(Exchange::IPrivacy::IPrivacySettingInfoIterator* &settings) const
    {
        LOGINFO("GetAllPrivacySettings");
        EssPrivacySettings privacySettings;
        uint32_t err = mEssClientPtr->GetPrivacySettings(privacySettings);
        if (err != Core::ERROR_NONE)
        {
            LOGERR("Failed to get all privacy settings: %d", err);
            Utils::Telemetry::SendError("Privacy: Failed to get all privacy settings: %d", err);
            if (err == Core::ERROR_GENERAL || err == Core::ERROR_NOT_SUPPORTED)
            {
                err = Core::ERROR_UNAVAILABLE;
            }
            return err;
        }

        std::map<const string, EssPrivacySettingData &> settingsMap =
        {
            {EssPrivacySettingContinueWatching, privacySettings.continueWatching},
            {EssPrivacySettingProductAnalytics, privacySettings.productAnalytics},
            {EssPrivacySettingWatchHistory, privacySettings.watchHistory},
            {EssPrivacySettingAcrCollection, privacySettings.acrCollection},
            {EssPrivacySettingAppContentAdTargeting, privacySettings.appContentAdTargeting},
            {EssPrivacySettingCameraAnalytics, privacySettings.cameraAnalytics},
            {EssPrivacySettingPersonalization, privacySettings.personalization},
            {EssPrivacySettingPrimaryBrowseAdTargeting, privacySettings.primaryBrowseAdTargeting},
            {EssPrivacySettingPrimaryContentAdTargeting, privacySettings.primaryContentAdTargeting},
            {EssPrivacySettingRemoteDiagnostics, privacySettings.remoteDiagnostics},
            {EssPrivacySettingUnentitledPersonalization, privacySettings.unentitledPersonalization},
            {EssPrivacySettingUnentitledContinueWatching, privacySettings.unentitledContinueWatching}
        };

        std::list<Exchange::IPrivacy::PrivacySettingInfo> privacySettingsList;
        for (auto &setting : settingsMap)
        {
            if (setting.second.available)
            {
                privacySettingsList.push_back({ setting.first, setting.second.allowed });
            }
        }

        settings = (Core::Service<RPC::IteratorType<Exchange::IPrivacy::IPrivacySettingInfoIterator>>::Create<Exchange::IPrivacy::IPrivacySettingInfoIterator>(privacySettingsList));

        return Core::ERROR_NONE;
    }

    Core::hresult PrivacyImplementation::GetExclusionPolicies(Exchange::IPrivacy::IStringIterator* &policies) const
    {
        LOGINFO("GetExclusionPolicies");
        std::unique_lock<std::mutex> lock(mExclusionPoliciesMutex);
        bool hasChanged = false;
        uint64_t now = GetCurrentTimestampInMs();
        uint64_t expirationTime = mLastExclusionPoliciesSuccesfullSyncTime + (mEssExclusionPoliciesSyncSec * 1000);

        if (expirationTime < now)
        {
            LOGINFO("Exclusion policies cache expired");
            EssExclusionPolicies newPolicies;
            uint32_t err = mEssClientPtr->GetExclusionPolicies(newPolicies);
            if (err != Core::ERROR_NONE)
            {
                LOGERR("Failed to get exclusion policies: %d", err);
                Utils::Telemetry::SendError("Privacy: Failed to get exclusion policies: %d", err);
                if (err == Core::ERROR_GENERAL || err == Core::ERROR_NOT_SUPPORTED)
                {
                    err = Core::ERROR_UNAVAILABLE;
                }
                return err;
            }
            mLastExclusionPoliciesSuccesfullSyncTime = now;
            if (mExclusionPolicies != newPolicies)
            {
                mExclusionPolicies = std::move(newPolicies);
                hasChanged = true;
            }
        }

        std::map<const string, EssExclusionPolicyData &> policiesMap =
        {
            {EssExclusionPolicyBusinessAnalytics, mExclusionPolicies.businessAnalytics},
            {EssExclusionPolicyPersonalization, mExclusionPolicies.personalization},
            {EssExclusionPolicyProductAnalytics, mExclusionPolicies.productAnalytics}
        };

        std::list<std::string> policiesList;

        for (auto const &policy : policiesMap)
        {
            if (policy.second.available)
            {
                policiesList.push_back(policy.first);
            }
        }

        policies = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(policiesList));

        if (hasChanged)
        {
            {
                std::unique_lock<std::mutex> lockQueue(mQueueMutex);
                Action action = {ACTION_TYPE_PROCESS_EXCLUSION_POLICY_CHANGED};
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        }

        return Core::ERROR_NONE;
    }

    Core::hresult PrivacyImplementation::GetDataForExclusionPolicy(const string& policy, IStringIterator* &dataEvents, IStringIterator* &entityReference, bool &derivativePropagation) const
    {
        LOGINFO("GetDataForExclusionPolicy");
        bool hasChanged = false;
        {
            std::unique_lock<std::mutex> lock(mExclusionPoliciesMutex);
            uint64_t now = GetCurrentTimestampInMs();
            uint64_t expirationTime = mLastExclusionPoliciesSuccesfullSyncTime + (mEssExclusionPoliciesSyncSec * 1000);
            if (expirationTime < now)
            {
                LOGINFO("Exclusion policies cache expired");
                EssExclusionPolicies newPolicies;
                uint32_t err = mEssClientPtr->GetExclusionPolicies(newPolicies);
                if (err != Core::ERROR_NONE)
                {
                    LOGERR("Failed to get exclusion policies: %d", err);
                    Utils::Telemetry::SendError("Privacy: Failed to get exclusion policies: %d", err);
                    if (err == Core::ERROR_GENERAL || err == Core::ERROR_NOT_SUPPORTED)
                    {
                        err = Core::ERROR_UNAVAILABLE;
                    }
                    return err;
                }
                mLastExclusionPoliciesSuccesfullSyncTime = now;
                if (mExclusionPolicies != newPolicies)
                {
                    mExclusionPolicies = std::move(newPolicies);
                    hasChanged = true;
                }
            }

            std::map<const string, EssExclusionPolicyData &> policiesMap =
                {
                    {EssExclusionPolicyBusinessAnalytics, mExclusionPolicies.businessAnalytics},
                    {EssExclusionPolicyPersonalization, mExclusionPolicies.personalization},
                    {EssExclusionPolicyProductAnalytics, mExclusionPolicies.productAnalytics}};

            if (policiesMap.find(policy) == policiesMap.end())
            {
                LOGERR("Invalid policy name");
                Utils::Telemetry::SendError("Privacy: Invalid policy name %s", policy.c_str());
                return Core::ERROR_UNAVAILABLE;
            }

            if (!policiesMap.at(policy).available)
            {
                LOGERR("Policy not available");
                Utils::Telemetry::SendError("Privacy: Policy %s not available", policy.c_str());
                return Core::ERROR_UNAVAILABLE;
            }

            dataEvents = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(policiesMap.at(policy).dataEvents));
            entityReference = (Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(policiesMap.at(policy).entityReference));
            derivativePropagation = policiesMap.at(policy).derivativePropagation;
        }

        if (hasChanged)
        {
            {
                std::unique_lock<std::mutex> lockQueue(mQueueMutex);
                Action action = {ACTION_TYPE_PROCESS_EXCLUSION_POLICY_CHANGED};
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        }

        return Core::ERROR_NONE;
    }


    void PrivacyImplementation::ActionLoop()
    {
        Init();

        std::unique_lock<std::mutex> lock(mQueueMutex);

        while (true) {

            std::chrono::milliseconds queueTimeout(std::chrono::milliseconds::max());

            if (!mInitialized)
            {
                queueTimeout = std::chrono::milliseconds(PRIVACY_INIT_RETRY_MS);
            }

            if (mActionQueue.empty())
            {
                if (queueTimeout == std::chrono::milliseconds::max())
                {
                    mQueueCondition.wait(lock, [this]
                                         { return !mActionQueue.empty(); });
                }
                else
                {
                    mQueueCondition.wait_for(lock, queueTimeout, [this]
                                             { return !mActionQueue.empty(); });
                }
            }

            Action action {ACTION_TYPE_INIT};
            if (!mActionQueue.empty())
            {
                action = mActionQueue.front();
                mActionQueue.pop();
            }

            lock.unlock();

            switch (action.type)
            {
            case ACTION_TYPE_INIT:
                {
                    if (mInitialized)
                    {
                        break;
                    }

                    LOGINFO("Initializing Privacy");
                    SyncConsentStrings();
                    SyncExclusionPolicies();
                    mInitialized = true;
                    break;
                }
            case ACTION_TYPE_SYNC_CONSENT_STRINGS:
                {
                    LOGINFO("Syncing consent strings");
                    SyncConsentStrings();
                    break;
                }
            case ACTION_TYPE_SYNC_EXCLUSION_POLICIES:
                {
                    LOGINFO("Syncing exclusion policies");
                    SyncExclusionPolicies();
                    break;
                }
            case ACTION_TYPE_SYNC_PRIVACY_SETTINGS:
                {
                    LOGINFO("Syncing privacy settings");
                    SyncPrivacySettings();
                    break;
                }
            case ACTION_TYPE_PROCESS_CONSENT_STRING_CHANGED:
                {
                    LOGINFO("Processing consent string changed");
                    std::unique_lock<std::mutex> lock(mConsentStringsMutex);
                    // Write fresh values to Persistent Store
                    SetConsentStringsCache(mConsentStringsMap);
                    // Notify all registered clients
                    DispatchEvent(CONSENT_STRING_CHANGED, false);
                    break;
                }
            case ACTION_TYPE_PROCESS_EXCLUSION_POLICY_CHANGED:
                {
                    LOGINFO("Processing exclusion policy changed");
                    std::unique_lock<std::mutex> lock(mExclusionPoliciesMutex);
                    // Write fresh values to Persistent Store
                    SetExclusionPoliciesCache(mExclusionPolicies, mLastExclusionPoliciesSuccesfullSyncTime);
                    // Notify all registered clients
                    DispatchEvent(EXCLUSION_POLICY_CHANGED, false);
                    break;
                }
            case ACTION_TYPE_SHUTDOWN:
                {
                    LOGINFO("Shutting down Privacy");
                    return;
                }
            default:
                break;
            }

            lock.lock();
        }
    }

    void PrivacyImplementation::Init()
    {
        LOGINFO("Init");

        // Register for cache change
        auto interface = mShell->QueryInterfaceByCallsign<Exchange::IStore>(PERSISTENT_STORE_CALLSIGN);
        if (interface == nullptr)
        {
            LOGERR("No IStore");
            return;
        }

        // coverity[missing_lock]
        uint32_t result = interface->Register(&mMonitorCache);
        interface->Release();
        if (result != Core::ERROR_NONE)
        {
            LOGERR("IStore status %d", result);
        }

        // Get current cache
        {
            std::unique_lock<std::mutex> lock(mConsentStringsMutex);
            GetConsentStringsFromCache(mConsentStringsMap);
        }

        {
            std::unique_lock<std::mutex> lock(mExclusionPoliciesMutex);
            GetExclusionPoliciesFromCache(mExclusionPolicies, mLastExclusionPoliciesSuccesfullSyncTime);
        }
    }

    void PrivacyImplementation::SyncConsentStrings()
    {
        LOGINFO("SyncConsentStrings");
        bool ret = false;
        uint32_t expireTime = 0;
        std::unique_lock<std::mutex> lock(mConsentStringsMutex);
        bool retry = false;

        // Read fresh values from ESS
        std::list<string> adUsecases = { EssConsentStringPrimaryContentAdTargeting, EssConsentStringPrimaryBrowseAdTargeting, EssConsentStringAppContentAdTargeting };
        std::unordered_map<string, EssConsentString> essConsentStringsMap;
        for (auto &adUsecase : adUsecases)
        {
            EssConsentString consentString;
            uint32_t err = mEssClientPtr->GetConsentString(adUsecase, consentString);
            if (err != Core::ERROR_NONE)
            {
                LOGERR("Failed to get consent string from ESS: %d", err);
                Utils::Telemetry::SendError("Privacy: Failed to get consent string from ESS: %d", err);
                if (err == Core::ERROR_UNREACHABLE_NETWORK)
                {
                    retry = true;
                }
            }
            else
            {
                // at least one consent string was retrieved - not all may be available
                ret = true;
                essConsentStringsMap[adUsecase] = std::move(consentString);
            }
        }

        bool isDifferent = false;
        for (auto &adUsecase : adUsecases)
        {
            // Compare with cached values
            if (essConsentStringsMap.find(adUsecase) != essConsentStringsMap.end()
                && mConsentStringsMap.find(adUsecase) != mConsentStringsMap.end())
            {
                if (essConsentStringsMap[adUsecase].gppValue != mConsentStringsMap[adUsecase].gppValue ||
                    essConsentStringsMap[adUsecase].gppSid != mConsentStringsMap[adUsecase].gppSid ||
                    essConsentStringsMap[adUsecase].decoded != mConsentStringsMap[adUsecase].decoded)
                {
                    isDifferent = true;
                    break;
                }
            }
            // Or if available from Ess but not in cache
            else if (essConsentStringsMap.find(adUsecase) != essConsentStringsMap.end()
                && mConsentStringsMap.find(adUsecase) == mConsentStringsMap.end())
            {
                isDifferent = true;
                break;
            }
        }

        mConsentStringsMap.clear();
        mConsentStringsMap = std::move(essConsentStringsMap);

        //lambda function to send sync request
        auto syncConsentStringsReq = [this] {
            {
                std::unique_lock<std::mutex> qlock(mQueueMutex);
                Action action = {ACTION_TYPE_SYNC_CONSENT_STRINGS};
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        };

        // Schedule next sync
        mConsentStringsSyncTimer.stop();
        mConsentStringsExpiredTimer.stop();
        if (retry && mConsentStringsRetryLimit > 0)
        {
            mConsentStringsRetryLimit--;
            mConsentStringsSyncTimer.start(mConsentStringsRetryBackoffTimeSec, syncConsentStringsReq);
            mConsentStringsRetryBackoffTimeSec *= 2;
            LOGINFO("Retrying consent strings sync in %d seconds", mConsentStringsRetryBackoffTimeSec);
            Utils::Telemetry::SendMessage("Privacy: Retrying consent strings sync in %d seconds", mConsentStringsRetryBackoffTimeSec);
        }
        else
        {
            // Ess sync was ok or limit reached, resync after 24 hours or expire time
            mConsentStringsRetryLimit = mEssRetryLimit;
            mConsentStringsRetryBackoffTimeSec = mEssBaseRetryBackoffTimeSec;
            mConsentStringsSyncTimer.start(PRIVACY_CONSENT_STRINGS_SYNC_INTERVAL_SEC, syncConsentStringsReq);
            LOGINFO("Resyncing consent strings in %d seconds", PRIVACY_CONSENT_STRINGS_SYNC_INTERVAL_SEC);
            Utils::Telemetry::SendMessage("Privacy: Resyncing consent strings in %d seconds", PRIVACY_CONSENT_STRINGS_SYNC_INTERVAL_SEC);
        }

        if (ret)
        {
            // If sync was ok, so we have fresh data, schedule expire time
            for (auto &adUsecase : adUsecases)
            {
                if (mConsentStringsMap.find(adUsecase) != mConsentStringsMap.end())
                {
                    if (mConsentStringsMap[adUsecase].expireTime > 0 && (expireTime == 0 || mConsentStringsMap[adUsecase].expireTime < expireTime))
                    {
                        expireTime = mConsentStringsMap[adUsecase].expireTime;
                    }
                }
            }
            if (expireTime > 0)
            {
                mConsentStringsExpiredTimer.start(expireTime, syncConsentStringsReq);
                LOGINFO("Resyncing consent strings on cache expire in %d seconds", expireTime);
                Utils::Telemetry::SendMessage("Privacy: Consent strings will expire in %d seconds", expireTime);
            }
        }

        if (isDifferent)
        {
            // Write fresh values to Persistent Store
            SetConsentStringsCache(mConsentStringsMap);
            // Notify all registered clients
            DispatchEvent(CONSENT_STRING_CHANGED, false);
        }
    }

    uint32_t PrivacyImplementation::GetConsentStringsFromCache(std::unordered_map<string, EssConsentString> &consentStringsMap)
    {
        // Read cached values from Persistent Store
        string cachedStringValue;
        uint32_t result;
        auto interface = mShell->QueryInterfaceByCallsign<Exchange::IStore>(PERSISTENT_STORE_CALLSIGN);
        if (interface == nullptr)
        {
            LOGERR("No IStore");
            return Core::ERROR_GENERAL;
        }

        result = interface->GetValue(PERSISTENT_STORE_PRIVACY_NAMESPACE, PERSISTENT_STORE_PRIVACY_CONSET_STRINGS_KEY, cachedStringValue);
        interface->Release();

        if (result != Core::ERROR_NONE)
        {
            // no cached values
            return result;
        }

        return ParseCachedConsentStrings(cachedStringValue, consentStringsMap);
    }

    uint32_t PrivacyImplementation::SetConsentStringsCache(const std::unordered_map<string, EssConsentString> &consentStringsMap)
    {
        //  PrivacyConsentStringsCache::ToString(str) doesn't work for Array with custom objects...
        //  so we need to create a new JsonObject from scratch
        JsonArray consentStringsArray;
        for (auto &adUsecase : consentStringsMap)
        {
            auto &consentString = adUsecase.second;
            JsonObject consentStringData;
            consentStringData["adUsecase"] = adUsecase.first;
            consentStringData["gppValue"] = consentString.gppValue;
            consentStringData["gppSid"] = consentString.gppSid;
            consentStringData["decoded"] = consentString.decoded;
            consentStringsArray.Add(consentStringData);
        }

        JsonObject consentStringsCache;
        consentStringsCache["consentStrings"] = consentStringsArray;

        string str;
        consentStringsCache.ToString(str);
        uint32_t result;
        auto interface = mShell->QueryInterfaceByCallsign<Exchange::IStore>(PERSISTENT_STORE_CALLSIGN);
        if (interface == nullptr)
        {
            LOGERR("No IStore");
            return Core::ERROR_GENERAL;
        }

        result = interface->SetValue(PERSISTENT_STORE_PRIVACY_NAMESPACE, PERSISTENT_STORE_PRIVACY_CONSET_STRINGS_KEY, str);
        interface->Release();

        return result;
    }

    uint32_t PrivacyImplementation::ParseCachedConsentStrings(const string &cache, std::unordered_map<string, EssConsentString> &consentStringsMap)
    {
        JsonObject consentStringsCache;
        Core::OptionalType<Core::JSON::Error> error;

        //  PrivacyConsentStringsCache::FromString(str) doesn't work for Array with custom objects...
        //  so we need to create a new JsonObject from scratch
        if (consentStringsCache.FromString(cache, error))
        {
            if (!consentStringsCache.HasLabel("consentStrings") ||
                consentStringsCache["consentStrings"].Content() != WPEFramework::Core::JSON::Variant::type::ARRAY)
            {
                LOGERR("No consent strings array in cache");
                return Core::ERROR_GENERAL;
            }

            JsonArray consentStrings = consentStringsCache["consentStrings"].Array();
            for (int i=0; i< consentStrings.Length(); ++i)
            {
                if (consentStrings[i].Content() != WPEFramework::Core::JSON::Variant::type::OBJECT)
                {
                    LOGERR("Consent string is not an object");
                    return Core::ERROR_GENERAL;
                }
                JsonObject consentString = consentStrings[i].Object();
                if (!consentString.HasLabel("adUsecase") || consentString["adUsecase"].Content() != WPEFramework::Core::JSON::Variant::type::STRING ||
                    !consentString.HasLabel("gppValue") || consentString["gppValue"].Content() != WPEFramework::Core::JSON::Variant::type::STRING ||
                    !consentString.HasLabel("gppSid") || consentString["gppSid"].Content() != WPEFramework::Core::JSON::Variant::type::STRING ||
                    !consentString.HasLabel("decoded") || consentString["decoded"].Content() != WPEFramework::Core::JSON::Variant::type::STRING)
                {
                    LOGERR("Consent string is not valid");
                    return Core::ERROR_GENERAL;
                }

                string adUsecase = consentString["adUsecase"].String();
                EssConsentString essConsentString;
                essConsentString.gppValue = consentString["gppValue"].String();
                essConsentString.gppSid = consentString["gppSid"].String();
                essConsentString.decoded = consentString["decoded"].String();
                consentStringsMap[adUsecase] = std::move(essConsentString);
            }
        }
        else
        {
            LOGERR("Failed to parse cached consent strings error: '%s', line: '%s'.", (error.IsSet() ? error.Value().Message().c_str() : "Unknown"),
                   cache.c_str());
            return Core::ERROR_GENERAL;
        }

        return Core::ERROR_NONE;
    }

    void PrivacyImplementation::SyncExclusionPolicies()
    {
        // Get fresh values from ESS
        LOGINFO("SyncExclusionPolicies");
        std::unique_lock<std::mutex> lock(mExclusionPoliciesMutex);
        // Read fresh values from ESS
        EssExclusionPolicies newPolicies;
        bool isDifferent = false;
        uint32_t err = mEssClientPtr->GetExclusionPolicies(newPolicies);
        if (err != Core::ERROR_NONE)
        {
            LOGERR("Failed to get exclusion policies from ESS: %d", err);
            Utils::Telemetry::SendError("Privacy: Failed to get exclusion policies from ESS: %d", err);
            if (err == Core::ERROR_UNREACHABLE_NETWORK
                && mExclusionPoliciesRetryLimit > 0)
            {
                mExclusionPoliciesSyncTimer.stop();
                mExclusionPoliciesSyncTimer.start(mExclusionPoliciesBackoffTimeSec, [this] {
                    {
                        std::unique_lock<std::mutex> qlock(mQueueMutex);
                        Action action = {ACTION_TYPE_SYNC_EXCLUSION_POLICIES};
                        mActionQueue.push(action);
                    }
                    mQueueCondition.notify_one();
                });
                mExclusionPoliciesRetryLimit--;
                mExclusionPoliciesBackoffTimeSec *= 2;
                return;
            }
        }
        else
        {
            //compare 
            if (mExclusionPolicies != newPolicies)
            {
                isDifferent = true;
                mExclusionPolicies = std::move(newPolicies);
            }
            mLastExclusionPoliciesSuccesfullSyncTime = GetCurrentTimestampInMs();
        }

        mExclusionPoliciesRetryLimit = mEssRetryLimit;
        mExclusionPoliciesBackoffTimeSec = mEssBaseRetryBackoffTimeSec;

        //lambda function to send sync request
        auto syncExclusionPoliciesReq = [this] {
            {
                std::unique_lock<std::mutex> qlock(mQueueMutex);
                Action action = {ACTION_TYPE_SYNC_EXCLUSION_POLICIES};
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        };

        // Schedule next sync
        mExclusionPoliciesSyncTimer.stop();
        mExclusionPoliciesSyncTimer.start(mEssExclusionPoliciesSyncSec, syncExclusionPoliciesReq);

        if (isDifferent)
        {
            // Write fresh values to Persistent Store
            SetExclusionPoliciesCache(mExclusionPolicies, mLastExclusionPoliciesSuccesfullSyncTime);
            // Notify all registered clients
            DispatchEvent(EXCLUSION_POLICY_CHANGED, false);
        }
    }

    uint32_t PrivacyImplementation::GetExclusionPoliciesFromCache(EssExclusionPolicies &policies, uint64_t &lastSyncTime)
    {
        // Read cached values from Persistent Store
        string cachedStringValue;
        uint32_t result;
        auto interface = mShell->QueryInterfaceByCallsign<Exchange::IStore>(PERSISTENT_STORE_CALLSIGN);
        if (interface == nullptr)
        {
            LOGERR("No IStore");
            return Core::ERROR_GENERAL;
        }

        result = interface->GetValue(PERSISTENT_STORE_PRIVACY_NAMESPACE, PERSISTENT_STORE_PRIVACY_EXCLUSION_POLICIES_KEY, cachedStringValue);
        interface->Release();

        if (result != Core::ERROR_NONE)
        {
            // no cached values
            return result;
        }

        return ParseCachedExclusionPolicies(cachedStringValue, policies, lastSyncTime);
    }

    uint32_t PrivacyImplementation::SetExclusionPoliciesCache(const EssExclusionPolicies &policies, const uint64_t &lastSyncTime)
    {
        JsonObject exclusionPoliciesCache;
        std::map<std::string, const EssExclusionPolicyData&> policiesData = {
            {"businessAnalytics", policies.businessAnalytics},
            {"personalization", policies.personalization},
            {"productAnalytics", policies.productAnalytics}
        };

        for (auto &policy : policiesData)
        {
            JsonObject policyData;
            if (policy.second.available)
            {
                JsonArray dataEventsArray;
                for (auto &dataEvent : policy.second.dataEvents)
                {
                    dataEventsArray.Add(dataEvent);
                }
                policyData["dataEvents"] = dataEventsArray;


                JsonArray entityReferenceArray;
                for (auto &entityRef : policy.second.entityReference)
                {
                    entityReferenceArray.Add(entityRef);
                }
                policyData["entityReference"] = entityReferenceArray;
                policyData["derivativePropagation"] = policy.second.derivativePropagation;

                exclusionPoliciesCache[policy.first.c_str()] = policyData;
            }
        }

        exclusionPoliciesCache["lastSyncTime"] = lastSyncTime;

        string str;
        exclusionPoliciesCache.ToString(str);
        uint32_t result;
        auto interface = mShell->QueryInterfaceByCallsign<Exchange::IStore>(PERSISTENT_STORE_CALLSIGN);
        if (interface == nullptr)
        {
            LOGERR("No IStore");
            return Core::ERROR_GENERAL;
        }

        result = interface->SetValue(PERSISTENT_STORE_PRIVACY_NAMESPACE, PERSISTENT_STORE_PRIVACY_EXCLUSION_POLICIES_KEY, str);
        interface->Release();

        return result;
    }

    uint32_t PrivacyImplementation::ParseCachedExclusionPolicies(const string &cache, EssExclusionPolicies &policies, uint64_t &lastSyncTime)
    {
        JsonObject exclusionPoliciesCache;
        Core::OptionalType<Core::JSON::Error> error;

        if (exclusionPoliciesCache.FromString(cache, error))
        {
            std::map<std::string, EssExclusionPolicyData&> policiesData = {
                {"businessAnalytics", policies.businessAnalytics},
                {"personalization", policies.personalization},
                {"productAnalytics", policies.productAnalytics}
            };

            policies.businessAnalytics.available = false;
            policies.personalization.available = false;
            policies.productAnalytics.available = false;
            lastSyncTime = 0;

            if (exclusionPoliciesCache.HasLabel("lastSyncTime") && exclusionPoliciesCache["lastSyncTime"].Content() == WPEFramework::Core::JSON::Variant::type::NUMBER)
            {
                lastSyncTime = exclusionPoliciesCache["lastSyncTime"].Number();
            }
            else
            {
                LOGERR("No last sync time in cache");
                Utils::Telemetry::SendError("Privacy: No last sync time in cache");
                return Core::ERROR_GENERAL;
            }

            for (auto &policy : policiesData)
            {
                if (exclusionPoliciesCache.HasLabel(policy.first.c_str())
                    && exclusionPoliciesCache[policy.first.c_str()].Content() == WPEFramework::Core::JSON::Variant::type::OBJECT)
                {
                    JsonObject policyData = exclusionPoliciesCache[policy.first.c_str()].Object();
                    if (policyData.HasLabel("dataEvents") && policyData["dataEvents"].Content() == WPEFramework::Core::JSON::Variant::type::ARRAY &&
                        policyData.HasLabel("entityReference") && policyData["entityReference"].Content() == WPEFramework::Core::JSON::Variant::type::ARRAY &&
                        policyData.HasLabel("derivativePropagation") && policyData["derivativePropagation"].Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN)
                    {
                        JsonArray dataEventsArray = policyData["dataEvents"].Array();
                        for (int i=0; i< dataEventsArray.Length(); ++i)
                        {
                            if (dataEventsArray[i].Content() != WPEFramework::Core::JSON::Variant::type::STRING)
                            {
                                LOGERR("Invalid data event for: %s", policy.first.c_str());
                                continue;
                            }
                            string dataEvent = dataEventsArray[i].String();
                            policy.second.dataEvents.insert(std::move(dataEvent));
                        }

                        JsonArray entityReferenceArray = policyData["entityReference"].Array();
                        for (int i=0; i< entityReferenceArray.Length(); ++i)
                        {
                            if (entityReferenceArray[i].Content() != WPEFramework::Core::JSON::Variant::type::STRING)
                            {
                                LOGERR("Invalid entity reference for : %s", policy.first.c_str());
                                continue;
                            }
                            string entityRef = entityReferenceArray[i].String();
                            policy.second.entityReference.insert(std::move(entityRef));
                        }

                        policy.second.derivativePropagation = policyData["derivativePropagation"].Boolean();
                        policy.second.available = true;
                    }
                    else
                    {
                        LOGERR("Invalid exclusion policies cache for : %s", policy.first.c_str());
                        Utils::Telemetry::SendError("Privacy: Invalid exclusion policies cache for: %s", policy.first.c_str());
                    }
                }
            }
        }
        else
        {
            LOGERR("Failed to parse cached exclusion policies error: '%s', line: '%s'.", (error.IsSet() ? error.Value().Message().c_str() : "Unknown"),
                   cache.c_str());
            return Core::ERROR_GENERAL;
        }
        return Core::ERROR_NONE;
    }

    uint64_t PrivacyImplementation::GetCurrentTimestampInMs() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>( std::chrono::system_clock::now().time_since_epoch() ).count();
    }

    void PrivacyImplementation::SyncPrivacySettings()
    {
        //lambda function to send sync request
        auto syncPrivacySettingsReq = [this] {
            {
                std::unique_lock<std::mutex> qlock(mQueueMutex);
                Action action = {ACTION_TYPE_SYNC_PRIVACY_SETTINGS};
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        };

        EssPrivacySettings privacySettings;
        uint32_t err = mEssClientPtr->GetPrivacySettings(privacySettings);

        std::unique_lock<std::mutex> lock(mPrivacySettingsMutex);
        if (err != Core::ERROR_NONE)
        {
            LOGERR("Failed to get all privacy settings: %d", err);
            Utils::Telemetry::SendError("Privacy: Failed to get all privacy settings: %d", err);
            
            if (err == Core::ERROR_UNREACHABLE_NETWORK)
            {
                if (mPrivacySettingsRetryLimit > 0)
                {
                    mPrivacySettingsRetryLimit--;
                    mPrivacySettingsSyncTimer.start(mPrivacySettingsBackoffTimeSec, syncPrivacySettingsReq);
                    mPrivacySettingsBackoffTimeSec *= 2;
                    LOGINFO("Retrying privacy settings sync in %d seconds", mPrivacySettingsBackoffTimeSec);
                    Utils::Telemetry::SendMessage("Privacy: Retrying privacy settings sync in %d seconds", mPrivacySettingsBackoffTimeSec);
                }
            }
            
            if (err == Core::ERROR_GENERAL || err == Core::ERROR_NOT_SUPPORTED)
            {
                err = Core::ERROR_UNAVAILABLE;
            }
        }
        else
        {
            // Sync successful
            mPrivacySettingsRetryLimit = mEssRetryLimit;
            mPrivacySettingsBackoffTimeSec = mEssBaseRetryBackoffTimeSec;
            if (mPrivacySettingsLinchPinEventsEnabled)
            {
                mPrivacySettingsCacheValid = true;
            }
            PrivacySettingsEmitEventIfNeeded(mPrivacySettings, privacySettings);

            mPrivacySettings = privacySettings;
        }
    }

    void PrivacyImplementation::PrivacySettingsEmitEventIfNeeded(const EssPrivacySettings &pOld, const EssPrivacySettings &pNew)
    {
        if (pOld.continueWatching != pNew.continueWatching && pNew.continueWatching.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_CONTINUE_WATCHING_CHANGED, pNew.continueWatching.allowed);
        }

        if (pOld.productAnalytics != pNew.productAnalytics && pNew.productAnalytics.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_PRODUCT_ANALYTICS_CHANGED, pNew.productAnalytics.allowed);
        }

        if (pOld.watchHistory != pNew.watchHistory && pNew.watchHistory.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_WATCH_HISTORY_CHANGED, pNew.watchHistory.allowed);
        }

        if (pOld.acrCollection != pNew.acrCollection && pNew.acrCollection.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_ACR_COLLECTION_CHANGED, pNew.acrCollection.allowed);
        }

        if (pOld.appContentAdTargeting != pNew.appContentAdTargeting && pNew.appContentAdTargeting.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_APP_CONTENT_AD_TARGETING_CHANGED, pNew.appContentAdTargeting.allowed);
        }

        if (pOld.cameraAnalytics != pNew.cameraAnalytics && pNew.cameraAnalytics.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_CAMERA_ANALYTICS_CHANGED, pNew.cameraAnalytics.allowed);
        }

        if (pOld.personalization != pNew.personalization && pNew.personalization.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_PERSONALIZATION_CHANGED, pNew.personalization.allowed);
        }

        if (pOld.primaryBrowseAdTargeting != pNew.primaryBrowseAdTargeting && pNew.primaryBrowseAdTargeting.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_PRIMARY_BROWSE_AD_TARGETING_CHANGED, pNew.primaryBrowseAdTargeting.allowed);
        }

        if (pOld.primaryContentAdTargeting != pNew.primaryContentAdTargeting && pNew.primaryContentAdTargeting.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_PRIMARY_CONTENT_AD_TARGETING_CHANGED, pNew.primaryContentAdTargeting.allowed);
        }

        if (pOld.remoteDiagnostics != pNew.remoteDiagnostics && pNew.remoteDiagnostics.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_REMOTE_DIAGNOSTICS_CHANGED, pNew.remoteDiagnostics.allowed);
        }

        if (pOld.unentitledPersonalization != pNew.unentitledPersonalization && pNew.unentitledPersonalization.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_UNENTITLED_PERSONALIZATION_CHANGED, pNew.unentitledPersonalization.allowed);
        }

        if (pOld.unentitledContinueWatching != pNew.unentitledContinueWatching && pNew.unentitledContinueWatching.available)
        {
            DispatchEvent(PrivacyImplementation::Event::SETTING_UNENTITLED_RESUME_POINTS_CHANGED, pNew.unentitledContinueWatching.allowed);
        }
    }

    void PrivacyImplementation::DispatchEvent(PrivacyImplementation::Event event, bool allowed)
    {
        Core::IWorkerPool::Instance().Submit(Job::Create(this, event, allowed));
    }

    void PrivacyImplementation::Dispatch(PrivacyImplementation::Event event, bool allowed)
    {
        LOGINFO("Dispatch: event = %d, allowed = %d\n", (int)event, allowed);
        Utils::Telemetry::SendMessage("Privacy: Dispatch: event = %d, allowed = %d\n", (int)event, allowed);
        std::unique_lock<std::mutex> lock(mNotificationMutex);
        std::list<Exchange::IPrivacy::INotification *>::const_iterator index(mNotifications.begin());

        while (index != mNotifications.end())
        {
            switch (event)
            {
            case CONSENT_STRING_CHANGED:
                (*index)->OnConsentStringChanged();
                break;
            case EXCLUSION_POLICY_CHANGED:
                (*index)->OnExclusionPolicyChanged();
                break;
            case SETTING_CONTINUE_WATCHING_CHANGED:
                (*index)->OnContinueWatchingChanged(allowed);
                break;
            case SETTING_PRODUCT_ANALYTICS_CHANGED:
                (*index)->OnProductAnalyticsChanged(allowed);
                break;
            case SETTING_WATCH_HISTORY_CHANGED:
                (*index)->OnWatchHistoryChanged(allowed);
                break;
            case SETTING_ACR_COLLECTION_CHANGED:
                (*index)->OnACRCollectionChanged(allowed);
                break;
            case SETTING_CAMERA_ANALYTICS_CHANGED:
                (*index)->OnCameraAnalyticsChanged(allowed);
                break;
            case SETTING_PERSONALIZATION_CHANGED:
                (*index)->OnPersonalizationChanged(allowed);
                break;
            case SETTING_PRIMARY_BROWSE_AD_TARGETING_CHANGED:
                (*index)->OnPrimaryBrowseAdTargetingChanged(allowed);
                break;
            case SETTING_PRIMARY_CONTENT_AD_TARGETING_CHANGED:
                (*index)->OnPrimaryContentAdTargetingChanged(allowed);
                break;
            case SETTING_REMOTE_DIAGNOSTICS_CHANGED:
                (*index)->OnRemoteDiagnosticsChanged(allowed);
                break;
            case SETTING_UNENTITLED_PERSONALIZATION_CHANGED:
                (*index)->OnUnentitledPersonalizationChanged(allowed);
                break;
            case SETTING_UNENTITLED_RESUME_POINTS_CHANGED:
                (*index)->OnUnentitledResumePointsChanged(allowed);
                break;
            default:
                break;
            }
            ++index;
        }
    }

    void PrivacyImplementation::LinchPinNotificationReceived(LinchPinNotificationType type, const string &value)
    {
        std::unique_lock<std::mutex> lock(mPrivacySettingsMutex);
        bool sync = false;
        switch(type)
        {
            case PrivacyImplementation::LinchPinNotificationType::CONNECTED:
            {
                LOGINFO("Privacy: Linchpin connected");
                // schedule sync
                sync = true;
                mPrivacySettingsLinchPinEventsEnabled = true;
                mPrivacySettingsCacheValid = false;
            } break;

            case PrivacyImplementation::LinchPinNotificationType::DISCONNECTED:
            {
                LOGINFO("Privacy: Linchpin disconnected");
                mPrivacySettingsLinchPinEventsEnabled = false;
                mPrivacySettingsCacheValid = false;
                mPrivacySettingsSyncTimer.stop();
            } break;

            case PrivacyImplementation::LinchPinNotificationType::EVENT:
            {
                LOGINFO("Privacy: Linchpin event received: %s", value.c_str());
                JsonObject json;
                json.FromString(value);
                if (json.HasLabel("connectionstate") &&
                    json["connectionstate"].Content() == WPEFramework::Core::JSON::Variant::type::OBJECT)
                {
                    JsonObject connectionState = json["connectionstate"].Object();
                    if (connectionState.HasLabel("state") &&
                        connectionState["state"].Content() == WPEFramework::Core::JSON::Variant::type::STRING && connectionState["state"].String() == "connected")
                    {
                        mPrivacySettingsLinchPinEventsEnabled = true;

                        if (json.HasLabel("notifications") &&
                            json["notifications"].Content() == WPEFramework::Core::JSON::Variant::type::ARRAY)
                        {
                            JsonArray notifications = json["notifications"].Array();

                            // Check if at least one item has string 'payload' and it consists of
                            // 'privacy-setting-state-change' or 'settings-state-change'
                            for (int i = 0; i < notifications.Length(); ++i)
                            {
                                JsonObject notification = notifications[i].Object();
                                if (notification.HasLabel("payload") &&
                                    notification["payload"].Content() == WPEFramework::Core::JSON::Variant::type::STRING)
                                {
                                    const auto &payload = notification["payload"].String();
                                    if (payload.find("privacy-setting-state-change") != std::string::npos || payload.find("settings-state-change") != std::string::npos)
                                    {
                                        // Privacy settings change detected
                                        LOGINFO("Privacy: LinchPin privacy settings change detected: %s", payload.c_str());
                                        sync = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        LOGWARN("Privacy: LinchPin state not connected");
                        mPrivacySettingsLinchPinEventsEnabled = false;
                        mPrivacySettingsCacheValid = false;
                    }
                }
                else
                {
                    LOGWARN("Privacy: LinchPin state not available");
                    mPrivacySettingsLinchPinEventsEnabled = false;
                    mPrivacySettingsCacheValid = false;
                }
            }
        }

        if (sync)
        {
            auto syncPrivacySettingsReq = [this]
            {
                {
                    std::unique_lock<std::mutex> qlock(mQueueMutex);
                    Action action = {ACTION_TYPE_SYNC_PRIVACY_SETTINGS};
                    mActionQueue.push(action);
                }
                mQueueCondition.notify_one();
            };

            int syncDelaySec = 2; // Same delay as in AS
            LOGINFO("Scheduling privacy settings sync in %d seconds", syncDelaySec);
            mPrivacySettingsSyncTimer.stop();
            mPrivacySettingsSyncTimer.start(syncDelaySec, syncPrivacySettingsReq);
        }
    }

    void PrivacyImplementation::MonitorKeys::ValueChanged(const string& ns, const string& key, const string& value)
    {
        auto it = mCallbacks.find(ns);
        if (it != mCallbacks.end())
        {
            auto it2 = it->second.find(key);
            if (it2 != it->second.end())
            {
                LOGINFO("ValueChanged %s, %s, %s",ns.c_str(), key.c_str(), value.c_str());
                it2->second(value);
            }
        }
    }

    void PrivacyImplementation::MonitorKeys::StorageExceeded()
    {
    }

    void PrivacyImplementation::MonitorKeys::RegisterCallback(const string& ns, const string& key, Callback callback)
    {
        mCallbacks[ns][key] = std::move(callback);
    }

}
}
