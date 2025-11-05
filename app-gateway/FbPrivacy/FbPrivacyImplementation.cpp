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
#include "FbPrivacyImplementation.h"
#include "UtilsLogging.h"
#include "UtilsCallsign.h"

#include <interfaces/IPrivacy.h>

#define PRIVACY_PLUGIN_CALLSIGN "org.rdk.Privacy"

namespace WPEFramework
{
    namespace Plugin
    {

        SERVICE_REGISTRATION(FbPrivacyImplementation, 1, 0);

        FbPrivacyImplementation::FbPrivacyImplementation() : mShell(nullptr),
            mNotificationMutex(),
            mNotifications(),
            mPrivacyNotification(this),
            mAppNotificationDelegate(nullptr)
        {
        }

        FbPrivacyImplementation::~FbPrivacyImplementation()
        {
            // Cleanup resources if needed
            if (mShell != nullptr)
            {
                auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_PLUGIN_CALLSIGN);
                if (privacy != nullptr)
                {
                    privacy->Unregister(&mPrivacyNotification);
                    privacy->Release();
                }

                mShell->Release();
                mShell = nullptr;
            }
        }

        uint32_t FbPrivacyImplementation::Configure(PluginHost::IShell *shell)
        {
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();


            auto appNotifications = shell->QueryInterfaceByCallsign<Exchange::IAppNotifications>(APP_NOTIFICATIONS_CALLSIGN);

            if (appNotifications == nullptr) {
                LOGERR("No App Notifications plugin available");
                return Core::ERROR_GENERAL;
            }

            mAppNotificationDelegate = std::make_shared<BaseEventDelegate>(appNotifications);
            appNotifications->Release();

            auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_PLUGIN_CALLSIGN);
            if (privacy == nullptr)
            {
                LOGERR("Failed to get IPrivacy interface");
                return Core::ERROR_GENERAL;
            }

            privacy->Register(&mPrivacyNotification);
            privacy->Release();

            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::Register(Exchange::IFbPrivacy::INotification *notification)
        {
            ASSERT(nullptr != notification);
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

        Core::hresult FbPrivacyImplementation::Unregister(Exchange::IFbPrivacy::INotification *notification)
        {
            Core::hresult status = Core::ERROR_GENERAL;
            ASSERT(nullptr != notification);
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

        Core::hresult FbPrivacyImplementation::AllowACRCollection(bool &value) const
        {
            LOGINFO("AllowACRCollection");
            value = Get(ALLOW_ACR_COLLECTION);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowAppContentAdTargeting(bool &value) const
        {
            // Missing in Privacy plugin, "xcal:appContentAdTargeting"
            LOGINFO("AllowAppContentAdTargeting");
            value = Get(ALLOW_APP_CONTENT_AD_TARGETING);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowCameraAnalytics(bool &value) const
        {
            // Missing in Privacy plugin, "xcal:cameraAnalytics"
            LOGINFO("AllowCameraAnalytics");
            value = Get(ALLOW_CAMERA_ANALYTICS);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowPersonalization(bool &value) const
        {
            LOGINFO("AllowPersonalization");
            value = Get(ALLOW_PERSONALIZATION);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowPrimaryBrowseAdTargeting(bool &value) const
        {
            LOGINFO("AllowPrimaryBrowseAdTargeting");
            value = Get(ALLOW_PRIMARY_BROWSE_AD_TARGETING);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowPrimaryContentAdTargeting(bool &value) const
        {
            LOGINFO("AllowPrimaryContentAdTargeting");
            value = Get(ALLOW_PRIMARY_CONTENT_AD_TARGETING);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowProductAnalytics(bool &value) const
        {
            LOGINFO("AllowProductAnalytics");
            value = Get(ALLOW_PRODUCT_ANALYTICS);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowRemoteDiagnostics(bool &value) const
        {
            LOGINFO("AllowRemoteDiagnostics");
            value = Get(ALLOW_REMOTE_DIAGNOSTICS);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowResumePoints(bool &value) const
        {
            LOGINFO("AllowResumePoints");
            value = Get(ALLOW_RESUME_POINTS);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowUnentitledPersonalization(bool &value) const
        {
            LOGINFO("AllowUnentitledPersonalization");
            value = Get(ALLOW_UNENTITLED_PERSONALIZATION);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowUnentitledResumePoints(bool &value) const
        {
            LOGINFO("AllowUnentitledResumePoints");
            value = Get(ALLOW_UNENTITLED_RESUME_POINTS);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::AllowWatchHistory(bool &value) const
        {
            LOGINFO("AllowWatchHistory");
            value = Get(ALLOW_WATCH_HISTORY);
            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::Settings(Exchange::IFbPrivacy::PrivacySettings &out) const
        {
            LOGINFO("Settings");
            out.allowACRCollection = Get(ALLOW_ACR_COLLECTION);
            out.allowAppContentAdTargeting = Get(ALLOW_APP_CONTENT_AD_TARGETING);
            out.allowCameraAnalytics = Get(ALLOW_CAMERA_ANALYTICS);
            out.allowPersonalization = Get(ALLOW_PERSONALIZATION);
            out.allowPrimaryBrowseAdTargeting = Get(ALLOW_PRIMARY_BROWSE_AD_TARGETING);
            out.allowPrimaryContentAdTargeting = Get(ALLOW_PRIMARY_CONTENT_AD_TARGETING);
            out.allowProductAnalytics = Get(ALLOW_PRODUCT_ANALYTICS);
            out.allowRemoteDiagnostics = Get(ALLOW_REMOTE_DIAGNOSTICS);
            out.allowResumePoints = Get(ALLOW_RESUME_POINTS);
            out.allowUnentitledPersonalization = Get(ALLOW_UNENTITLED_PERSONALIZATION);
            out.allowUnentitledResumePoints = Get(ALLOW_UNENTITLED_RESUME_POINTS);
            out.allowWatchHistory = Get(ALLOW_WATCH_HISTORY);

            return Core::ERROR_NONE;
        }

        Core::hresult FbPrivacyImplementation::SetAllowACRCollection(const bool value)
        {
            LOGINFO("SetAllowACRCollection: %s", value ? "true" : "false");
            return Set(ALLOW_ACR_COLLECTION, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowAppContentAdTargeting(const bool value)
        {
            LOGINFO("SetAllowAppContentAdTargeting: %s", value ? "true" : "false");
            return Set(ALLOW_APP_CONTENT_AD_TARGETING, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowCameraAnalytics(const bool value)
        {
            LOGINFO("SetAllowCameraAnalytics: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:cameraAnalytics"
            return Set(ALLOW_CAMERA_ANALYTICS, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowPersonalization(const bool value)
        {
            LOGINFO("SetAllowPersonalization: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:personalization"
            return Set(ALLOW_PERSONALIZATION, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowPrimaryBrowseAdTargeting(const bool value)
        {
            LOGINFO("SetAllowPrimaryBrowseAdTargeting: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:primaryBrowseAdTargeting"
            return Set(ALLOW_PRIMARY_BROWSE_AD_TARGETING, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowPrimaryContentAdTargeting(const bool value)
        {
            LOGINFO("SetAllowPrimaryContentAdTargeting: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:primaryContentAdTargeting"
            return Set(ALLOW_PRIMARY_CONTENT_AD_TARGETING, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowProductAnalytics(const bool value)
        {
            LOGINFO("SetAllowProductAnalytics: %s", value ? "true" : "false");
            // Privacy.SetProductAnalytics
            return Set(ALLOW_PRODUCT_ANALYTICS, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowRemoteDiagnostics(const bool value)
        {
            LOGINFO("SetAllowRemoteDiagnostics: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:remoteDiagnostics"
            return Set(ALLOW_REMOTE_DIAGNOSTICS, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowResumePoints(const bool value)
        {
            LOGINFO("SetAllowResumePoints: %s", value ? "true" : "false");
            // Privacy.SetContinueWatching
            return Set(ALLOW_RESUME_POINTS, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowUnentitledPersonalization(const bool value)
        {
            LOGINFO("SetAllowUnentitledPersonalization: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:unentitledPersonalization"
            return Set(ALLOW_UNENTITLED_PERSONALIZATION, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowUnentitledResumePoints(const bool value)
        {
            LOGINFO("SetAllowUnentitledResumePoints: %s", value ? "true" : "false");
            // Missing in Privacy plugin, "xcal:unentitledContinueWatching"
            return Set(ALLOW_UNENTITLED_RESUME_POINTS, value);
        }

        Core::hresult FbPrivacyImplementation::SetAllowWatchHistory(const bool value)
        {
            LOGINFO("SetAllowWatchHistory: %s", value ? "true" : "false");
            return Set(ALLOW_WATCH_HISTORY, value);
        }

        Core::hresult FbPrivacyImplementation::HandleAppEventNotifier(const string& event /* @in */,
                const bool& listen /* @in */, 
                bool& status /* @out */)
        {
            if (mAppNotificationDelegate == nullptr) {
                LOGERR("App Notification Delegate is null");
                status = false;
                return Core::ERROR_GENERAL;
            }

            status = true;

            if (listen == false)
            {
                if (mAppNotificationDelegate->IsNotificationRegistered(event))
                {
                    mAppNotificationDelegate->RemoveNotification(event);
                }
            }
            else
            {
                if (!mAppNotificationDelegate->IsNotificationRegistered(event))
                {
                    mAppNotificationDelegate->AddNotification(event);
                }
            }

            return Core::ERROR_NONE;
        }

        void FbPrivacyImplementation::DispatchEvent(Setting setting, bool allowed)
        {
            Core::IWorkerPool::Instance().Submit(Job::Create(this, setting, allowed));
            if (mAppNotificationDelegate)
            {
                string event;
                switch (setting)
                {
                    case ALLOW_ACR_COLLECTION:
                        event = "Privacy.onAllowACRCollectionChanged";
                        break;
                    case ALLOW_APP_CONTENT_AD_TARGETING:
                        event = "Privacy.onAllowAppContentAdTargetingChanged";
                        break;
                    case ALLOW_CAMERA_ANALYTICS:
                        event = "Privacy.onAllowCameraAnalyticsChanged";
                        break;
                    case ALLOW_PERSONALIZATION:
                        event = "Privacy.onAllowPersonalizationChanged";
                        break;
                    case ALLOW_PRIMARY_BROWSE_AD_TARGETING:
                        event = "Privacy.onAllowPrimaryBrowseAdTargetingChanged";
                        break;
                    case ALLOW_PRIMARY_CONTENT_AD_TARGETING:
                        event = "Privacy.onAllowPrimaryContentAdTargetingChanged";
                        break;
                    case ALLOW_PRODUCT_ANALYTICS:
                        event = "Privacy.onAllowProductAnalyticsChanged";
                        break;
                    case ALLOW_REMOTE_DIAGNOSTICS:
                        event = "Privacy.onAllowRemoteDiagnosticsChanged";
                        break;
                    case ALLOW_RESUME_POINTS:
                        event = "Privacy.onAllowResumePointsChanged";
                        break;
                    case ALLOW_UNENTITLED_PERSONALIZATION:
                        event = "Privacy.onAllowUnentitledPersonalizationChanged";
                        break;
                    case ALLOW_UNENTITLED_RESUME_POINTS:
                        event = "Privacy.onAllowUnentitledResumePointsChanged";
                        break;
                    case ALLOW_WATCH_HISTORY:
                        event = "Privacy.onAllowWatchHistoryChanged";
                        break;
                    default:
                        LOGERR("Unknown setting: %d", (int)setting);
                        return;
                }

                mAppNotificationDelegate->Dispatch(event, allowed ? "true" : "false");
            }
        }

        void FbPrivacyImplementation::Dispatch(Setting setting, bool allowed)
        {
            LOGINFO("Dispatch: setting = %d, allowed = %d\n", (int)setting, allowed);
            std::unique_lock<std::mutex> lock(mNotificationMutex);
            std::list<Exchange::IFbPrivacy::INotification *>::const_iterator index(mNotifications.begin());

            while (index != mNotifications.end())
            {
                switch (setting)
                {
                case ALLOW_ACR_COLLECTION:
                    (*index)->OnAllowACRCollectionChanged(allowed);
                    break;
                case ALLOW_APP_CONTENT_AD_TARGETING:
                    (*index)->OnAllowAppContentAdTargetingChanged(allowed);
                    break;
                case ALLOW_CAMERA_ANALYTICS:
                    (*index)->OnAllowCameraAnalyticsChanged(allowed);
                    break;
                case ALLOW_PERSONALIZATION:
                    (*index)->OnAllowPersonalizationChanged(allowed);
                    break;
                case ALLOW_PRIMARY_BROWSE_AD_TARGETING:
                    (*index)->OnAllowPrimaryBrowseAdTargetingChanged(allowed);
                    break;
                case ALLOW_PRIMARY_CONTENT_AD_TARGETING:
                    (*index)->OnAllowPrimaryContentAdTargetingChanged(allowed);
                    break;
                case ALLOW_PRODUCT_ANALYTICS:
                    (*index)->OnAllowProductAnalyticsChanged(allowed);
                    break;
                case ALLOW_REMOTE_DIAGNOSTICS:
                    (*index)->OnAllowRemoteDiagnosticsChanged(allowed);
                    break;
                case ALLOW_RESUME_POINTS:
                    (*index)->OnAllowResumePointsChanged(allowed);
                    break;
                case ALLOW_UNENTITLED_PERSONALIZATION:
                    (*index)->OnAllowUnentitledPersonalizationChanged(allowed);
                    break;
                case ALLOW_UNENTITLED_RESUME_POINTS:
                    (*index)->OnAllowUnentitledResumePointsChanged(allowed);
                    break;
                case ALLOW_WATCH_HISTORY:
                    (*index)->OnAllowWatchHistoryChanged(allowed);
                    break;
                default:
                    LOGERR("Unknown setting: %d", (int)setting);
                    break;
                }
                ++index;
            }
        }

        uint32_t FbPrivacyImplementation::Set(Setting setting, bool allowed)
        {
            uint32_t result = Core::ERROR_UNAVAILABLE;
            if (mShell != nullptr)
            {
                auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_PLUGIN_CALLSIGN);
                if (privacy != nullptr)
                {
                    switch(setting)
                    {
                        case ALLOW_ACR_COLLECTION:
                            result = privacy->SetACRCollection(allowed);
                            break;
                        case ALLOW_APP_CONTENT_AD_TARGETING:
                            result = privacy->SetAppContentAdTargeting(allowed);
                            break;
                        case ALLOW_CAMERA_ANALYTICS:
                            result = privacy->SetCameraAnalytics(allowed);
                            break;
                        case ALLOW_PERSONALIZATION:
                            result = privacy->SetPersonalization(allowed);
                            break;
                        case ALLOW_PRIMARY_BROWSE_AD_TARGETING:
                            result = privacy->SetPrimaryBrowseAdTargeting(allowed);
                            break;
                        case ALLOW_PRIMARY_CONTENT_AD_TARGETING:
                            result = privacy->SetPrimaryContentAdTargeting(allowed);
                            break;
                        case ALLOW_PRODUCT_ANALYTICS:
                            result = privacy->SetProductAnalytics(allowed);
                            break;
                        case ALLOW_REMOTE_DIAGNOSTICS:
                            result = privacy->SetRemoteDiagnostics(allowed);
                            break;
                        case ALLOW_RESUME_POINTS:
                            result = privacy->SetContinueWatching(allowed);
                            break;
                        case ALLOW_UNENTITLED_PERSONALIZATION:
                            result = privacy->SetUnentitledPersonalization(allowed);
                            break;
                        case ALLOW_UNENTITLED_RESUME_POINTS:
                            result = privacy->SetUnentitledResumePoints(allowed);
                            break;
                        case ALLOW_WATCH_HISTORY:
                            result = privacy->SetWatchHistory(allowed);
                            break;
                        default:
                            LOGERR("Unknown setting: %d", (int)setting);
                            break;
                    }
                    privacy->Release();
                }
                else
                {
                    LOGERR("Privacy plugin not found");
                }
            }
            else
            {
                LOGERR("Shell not set");
            }

            return result;
        }

        bool FbPrivacyImplementation::Get(Setting setting) const
        {
            bool allowed = false;
            if (mShell != nullptr)
            {
                auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_PLUGIN_CALLSIGN);
                if (privacy != nullptr)
                {
                    Exchange::IPrivacy::PrivacySettingOutData data;
                    Core::hresult result = Core::ERROR_UNAVAILABLE;
                    switch(setting)
                    {
                        case ALLOW_ACR_COLLECTION:
                            result = privacy->GetACRCollection(data);
                            break;
                        case ALLOW_APP_CONTENT_AD_TARGETING:
                            result = privacy->GetAppContentAdTargeting(data);
                            break;
                        case ALLOW_CAMERA_ANALYTICS:
                            result = privacy->GetCameraAnalytics(data);
                            break;
                        case ALLOW_PERSONALIZATION:
                            result = privacy->GetPersonalization(data);
                            break;
                        case ALLOW_PRIMARY_BROWSE_AD_TARGETING:
                            result = privacy->GetPrimaryBrowseAdTargeting(data);
                            break;
                        case ALLOW_PRIMARY_CONTENT_AD_TARGETING:
                            result = privacy->GetPrimaryContentAdTargeting(data);
                            break;
                        case ALLOW_PRODUCT_ANALYTICS:
                            result = privacy->GetProductAnalytics(data);
                            break;
                        case ALLOW_REMOTE_DIAGNOSTICS:
                            result = privacy->GetRemoteDiagnostics(data);
                            break;
                        case ALLOW_RESUME_POINTS:
                            result = privacy->GetContinueWatching(data);
                            break;
                        case ALLOW_UNENTITLED_PERSONALIZATION:
                            result = privacy->GetUnentitledPersonalization(data);
                            break;
                        case ALLOW_UNENTITLED_RESUME_POINTS:
                            result = privacy->GetUnentitledResumePoints(data);
                            break;
                        case ALLOW_WATCH_HISTORY:
                            result = privacy->GetWatchHistory(data);
                            break;
                        default:
                            LOGERR("Unknown setting: %d", (int)setting);
                            break;
                    }
                    if (result == Core::ERROR_NONE)
                    {
                        allowed = data.allowed;
                    }
                    else
                    {
                        LOGERR("Failed to get setting: %d, error: %d", (int)setting, result);
                    }
                    privacy->Release();
                }
                else
                {
                    LOGERR("Privacy plugin not found");
                }
            }
            else
            {
                LOGERR("Shell not set");
            }

            return allowed;
        }

    } // namespace Plugin
} // namespace WPEFramework
