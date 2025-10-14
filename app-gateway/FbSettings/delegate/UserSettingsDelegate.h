 /**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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
**/
#ifndef __USERSETTINGSDELEGATE_H__
#define __USERSETTINGSDELEGATE_H__
#include "StringUtils.h"
#include "BaseEventDelegate.h"
#include <interfaces/IUserSettings.h>
#include <interfaces/ITextToSpeech.h>
#include "UtilsLogging.h"
#include "ObjectUtils.h"
#include <set>
#include <string>
using namespace WPEFramework;
#define USERSETTINGS_CALLSIGN "org.rdk.UserSettings"

static const std::set<std::string> VALID_USER_SETTINGS_EVENT = {
    "localization.onlanguagechanged",
    "localization.onlocalechanged",
    "localization.onpreferredaudiolanguageschanged",
    "accessibility.onaudiodescriptionsettingschanged",
    "accessibility.onhighcontrastuichanged",
    "closedcaptions.onenabledchanged",
    "closedcaptions.onpreferredlanguageschanged",
    "accessibility.onclosedcaptionssettingschanged",
    "accessibility.onvoiceguidancesettingschanged",
};

class UserSettingsDelegate : public BaseEventDelegate{
    public:
        UserSettingsDelegate(PluginHost::IShell* shell,Exchange::IAppNotifications* appNotifications):BaseEventDelegate(appNotifications), mShell(shell),mNotificationHandler(*this){}

        ~UserSettingsDelegate() {
            if (mUserSettings != nullptr) {
                mUserSettings->Release();
                mUserSettings = nullptr;
            }
        }

        bool HandleSubscription(const std::string& event, const bool listen) {
            if (listen) {
                if (mShell != nullptr) {
                    mUserSettings = mShell->QueryInterfaceByCallsign<Exchange::IUserSettings>(USERSETTINGS_CALLSIGN);
                    if (mUserSettings != nullptr) {
                        mUserSettings->AddRef();
                    }
                } else {
                    LOGERR("mShell is null exiting");
                    return false;
                }

                if (mUserSettings == nullptr) {
                    LOGERR("mUserSettings interface not available");
                    return false;
                }
                AddNotification(event);

                if (!mNotificationHandler.GetRegistered()) {
                    LOGINFO("Registering for UserSettings notifications");
                    mUserSettings->Register(&mNotificationHandler);
                    mNotificationHandler.SetRegistered(true);
                    return true;
                } else {
                    LOGTRACE(" Is UserSettings registered = %s", mNotificationHandler.GetRegistered() ? "true" : "false");
                }
                
            } else {
                // Not removing the notification subscription for cases where only one event is removed 
                // Registration is lazy one but we need to evaluate if there is any value in unregistering
                // given these API calls are always made
                RemoveNotification(event);
            }
            return false;
        }

        bool HandleEvent(const std::string& event, const bool listen, bool& registrationError) override {
            LOGDBG("Checking for handle event");
            // Check if event is present in VALID_USER_SETTINGS_EVENT make check case insensitive
            if (VALID_USER_SETTINGS_EVENT.find(StringUtils::toLower(event)) != VALID_USER_SETTINGS_EVENT.end()) {
                // Handle TextToSpeech event
                registrationError = HandleSubscription(event, listen);
                return true;
            }
            return false;
        }

        // -------------------------
        // PUBLIC_INTERFACE methods exposed to FbSettingsImplementation
        // -------------------------

        // PUBLIC_INTERFACE
        Core::hresult GetLanguage(std::string& language) {
            language.clear();
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            std::string locale;
            if (us->GetPresentationLanguage(locale) != Core::ERROR_NONE) {
                return Core::ERROR_UNAVAILABLE;
            }
            // Take the language subtag (e.g., "en-US" -> "en")
            const auto pos = locale.find('-');
            language = (pos == std::string::npos) ? locale : locale.substr(0, pos);
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnLanguageChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetLocale(std::string& locale) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->GetPresentationLanguage(locale);
        }

        // PUBLIC_INTERFACE
        Core::hresult SetLocale(const std::string& locale) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->SetPresentationLanguage(locale);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnLocaleChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetPreferredAudioLanguages(std::string& languages) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->GetPreferredAudioLanguages(languages);
        }

        // PUBLIC_INTERFACE
        Core::hresult SetPreferredAudioLanguages(const std::string& languages) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->SetPreferredAudioLanguages(languages);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnPreferredAudioLanguagesChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceEnabled(const bool enabled) {
            auto tts = AcquireTTS();
            if (!tts) { return Core::ERROR_UNAVAILABLE; }
            return tts->Enable(enabled);
        }

        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceSpeed(const int speed) {
            // Map speed to textual or numeric rate using TTS configuration
            auto tts = AcquireTTS();
            if (!tts) { return Core::ERROR_UNAVAILABLE; }
            Exchange::ITextToSpeech::Configuration cfg{};
            auto rc = tts->GetConfiguration(cfg);
            if (rc != Core::ERROR_NONE) return rc;
            // interpret speed [1..5] -> string "x-slow","slow","medium","fast","x-fast" if needed; here store numeric rate
            uint8_t clamped = static_cast<uint8_t>(std::max(0, std::min(100, speed)));
            cfg.rate = clamped;
            Exchange::ITextToSpeech::TTSErrorDetail detail;
            return tts->SetConfiguration(cfg, detail);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnAudioDescriptionSettingsChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetAudioDescriptionSettings(std::string& settingsJson) {
            bool enabled = false;
            auto rc = GetAudioDescriptionsEnabled(enabled);
            if (rc != Core::ERROR_NONE) return rc;
            settingsJson = ObjectUtils::CreateBooleanJsonString("enabled", enabled);
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetAudioDescriptionsEnabled(bool& enabled) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->GetAudioDescription(enabled);
        }

        // PUBLIC_INTERFACE
        Core::hresult SetAudioDescriptionsEnabled(const bool enabled) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->SetAudioDescription(enabled);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnAudioDescriptionsEnabledChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetHighContrastUI(bool& enabled) {
            // No explicit getter available in current IUserSettings; default to false
            enabled = false;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnHighContrastUIChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetClosedCaptionsEnabled(bool& enabled) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->GetCaptions(enabled);
        }

        // PUBLIC_INTERFACE
        Core::hresult SetClosedCaptionsEnabled(const bool enabled) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->SetCaptions(enabled);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnClosedCaptionsEnabledChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetClosedCaptionsPreferredLanguages(std::string& languages) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->GetPreferredCaptionsLanguages(languages);
        }

        // PUBLIC_INTERFACE
        Core::hresult SetClosedCaptionsPreferredLanguages(const std::string& languages) {
            auto us = AcquireUserSettings();
            if (!us) { return Core::ERROR_UNAVAILABLE; }
            return us->SetPreferredCaptionsLanguages(languages);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnClosedaptionsPreferredLanguagesChanged(const bool listen, bool& status) {
            // Intentional typo in method name to preserve alias naming match
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnClosedCaptionsSettingsChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceNavigationHints(bool& enabled) {
            // No direct API in ITextToSpeech; default to false
            enabled = false;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceNavigationHints(const bool enabled) {
            VARIABLE_IS_NOT_USED enabled;
            // No direct API; acknowledge request
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceNavigationHintsChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceRate(double& rate) {
            auto tts = AcquireTTS();
            if (!tts) { return Core::ERROR_UNAVAILABLE; }
            Exchange::ITextToSpeech::Configuration cfg{};
            auto rc = tts->GetConfiguration(cfg);
            if (rc != Core::ERROR_NONE) return rc;
            rate = static_cast<double>(cfg.rate);
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceRate(const double rate) {
            auto tts = AcquireTTS();
            if (!tts) { return Core::ERROR_UNAVAILABLE; }
            Exchange::ITextToSpeech::Configuration cfg{};
            auto rc = tts->GetConfiguration(cfg);
            if (rc != Core::ERROR_NONE) return rc;
            uint8_t clamped = static_cast<uint8_t>(std::max(0.0, std::min(100.0, rate)));
            cfg.rate = clamped;
            Exchange::ITextToSpeech::TTSErrorDetail detail;
            return tts->SetConfiguration(cfg, detail);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceRateChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceEnabled(bool& enabled) {
            auto tts = AcquireTTS();
            if (!tts) { return Core::ERROR_UNAVAILABLE; }
            return tts->Enable(enabled);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceEnabledChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceSpeed(int& speed) {
            double r = 0.0;
            auto rc = GetVoiceGuidanceRate(r);
            if (rc != Core::ERROR_NONE) return rc;
            speed = static_cast<int>(r);
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceSpeedChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceSettingsChanged(const bool listen, bool& status) {
            VARIABLE_IS_NOT_USED listen;
            status = true;
            return Core::ERROR_NONE;
        }

    private:
        inline Exchange::IUserSettings* AcquireUserSettings() {
            if (mUserSettings == nullptr && mShell != nullptr) {
                mUserSettings = mShell->QueryInterfaceByCallsign<Exchange::IUserSettings>(USERSETTINGS_CALLSIGN);
                if (mUserSettings != nullptr) {
                    mUserSettings->AddRef();
                }
            }
            return mUserSettings;
        }

        inline Exchange::ITextToSpeech* AcquireTTS() {
            if (mTextToSpeech == nullptr && mShell != nullptr) {
                mTextToSpeech = mShell->QueryInterfaceByCallsign<Exchange::ITextToSpeech>(TTS_CALLSIGN);
                if (mTextToSpeech != nullptr) {
                    mTextToSpeech->AddRef();
                }
            }
            return mTextToSpeech;
        }

        class UserSettingsNotificationHandler: public Exchange::IUserSettings::INotification {
            public:
                 UserSettingsNotificationHandler(UserSettingsDelegate& parent) : mParent(parent),registered(false){}
                ~UserSettingsNotificationHandler(){}

        void OnAudioDescriptionChanged(const bool enabled) {
            mParent.Dispatch( "accessibility.onaudiodescriptionsettingschanged", ObjectUtils::CreateBooleanJsonString("enabled", enabled));
        }

        void OnPreferredAudioLanguagesChanged(const std::string& preferredLanguages) {
            mParent.Dispatch( "localization.onpreferredaudiolanguageschanged", preferredLanguages);
        }

        void OnPresentationLanguageChanged(const std::string& presentationLanguage) {
            
            mParent.Dispatch( "localization.onlocalechanged", presentationLanguage);

            // check presentationLanguage is a delimitted string like "en-US"
            // add logic to get the "en" if the value is "en-US"
            if (presentationLanguage.find('-') != std::string::npos) {
                std::string language = presentationLanguage.substr(0, presentationLanguage.find('-'));
                mParent.Dispatch( "localization.onlanguagechanged", language);
            } else {
                LOGWARN("invalid value=%s set it must be a delimited string like en-US", presentationLanguage.c_str());
            }
        }

        void OnCaptionsChanged(const bool enabled) {
            mParent.Dispatch( "accessibility.onclosedcaptionssettingschanged", ObjectUtils::CreateBooleanJsonString("enabled", enabled));
        }

        void OnPreferredCaptionsLanguagesChanged(const std::string& preferredLanguages) {
            mParent.Dispatch( "closedcaptions.onpreferredlanguageschanged", preferredLanguages);
        }

        void OnPreferredClosedCaptionServiceChanged(const std::string& service) {
            mParent.Dispatch( "OnPreferredClosedCaptionServiceChanged", service);
        }

        void OnPrivacyModeChanged(const std::string& privacyMode) {
            mParent.Dispatch( "OnPrivacyModeChanged", privacyMode);
        }

        void OnPinControlChanged(const bool pinControl) {
            mParent.Dispatch( "OnPinControlChanged", ObjectUtils::BoolToJsonString(pinControl));
        }

        void OnViewingRestrictionsChanged(const std::string& viewingRestrictions) {
            mParent.Dispatch( "OnViewingRestrictionsChanged", viewingRestrictions);
        }

        void OnViewingRestrictionsWindowChanged(const std::string& viewingRestrictionsWindow) {
            mParent.Dispatch( "OnViewingRestrictionsWindowChanged", viewingRestrictionsWindow);
        }

        void OnLiveWatershedChanged(const bool liveWatershed) {
            mParent.Dispatch( "OnLiveWatershedChanged", ObjectUtils::BoolToJsonString(liveWatershed));
        }

        void OnPlaybackWatershedChanged(const bool playbackWatershed) {
            mParent.Dispatch( "OnPlaybackWatershedChanged", ObjectUtils::BoolToJsonString(playbackWatershed));
        }

        void OnBlockNotRatedContentChanged(const bool blockNotRatedContent) {
            mParent.Dispatch( "OnBlockNotRatedContentChanged", ObjectUtils::BoolToJsonString(blockNotRatedContent));
        }

        void OnPinOnPurchaseChanged(const bool pinOnPurchase) {
            mParent.Dispatch( "OnPinOnPurchaseChanged", ObjectUtils::BoolToJsonString(pinOnPurchase));
        }

        void OnHighContrastChanged(const bool enabled) {
            mParent.Dispatch( "accessibility.onhighcontrastuichanged", ObjectUtils::BoolToJsonString(enabled));
        }

        void OnVoiceGuidanceChanged(const bool enabled) {
            mParent.Dispatch( "accessibility.onvoiceguidancesettingschanged", ObjectUtils::CreateBooleanJsonString("enabled", enabled));
        }

        void OnVoiceGuidanceRateChanged(const double rate) {
            mParent.Dispatch( "OnVoiceGuidanceRateChanged", std::to_string(rate));
        }

        void OnVoiceGuidanceHintsChanged(const bool hints) {
            mParent.Dispatch( "OnVoiceGuidanceHintsChanged", std::to_string(hints));
        }

        void OnContentPinChanged(const std::string& contentPin) {
            mParent.Dispatch( "OnContentPinChanged", contentPin);
        }
                
                // New Method for Set registered
                void SetRegistered(bool state) {
                    std::lock_guard<std::mutex> lock(registerMutex);
                    registered = state;
                }

                // New Method for get registered
                bool GetRegistered() {
                    std::lock_guard<std::mutex> lock(registerMutex);
                    return registered;
                }

                BEGIN_INTERFACE_MAP(NotificationHandler)
                INTERFACE_ENTRY(Exchange::IUserSettings::INotification)
                END_INTERFACE_MAP
            private:
                    UserSettingsDelegate& mParent;
                    bool registered;
                    std::mutex registerMutex;

        };
        Exchange::IUserSettings *mUserSettings { nullptr };
        Exchange::ITextToSpeech *mTextToSpeech { nullptr };
        PluginHost::IShell* mShell;
        Core::Sink<UserSettingsNotificationHandler> mNotificationHandler;

        
};
#endif
