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
#include <interfaces/IFbSettings.h>
#include <interfaces/IConfiguration.h>
#include <mutex>
#include <map>
#include "UtilsLogging.h"
#include "ThunderUtils.h"
#include "delegate/SettingsDelegate.h"
#include "delegate/UserSettingsDelegate.h"

namespace WPEFramework {
namespace Plugin {
    class FbSettingsImplementation : public Exchange::IFbSettings, public Exchange::IConfiguration {
    private:

        FbSettingsImplementation(const FbSettingsImplementation&) = delete;
        FbSettingsImplementation& operator=(const FbSettingsImplementation&) = delete;

        class EXTERNAL EventRegistrationJob : public Core::IDispatch 
        {
            protected:
                EventRegistrationJob(FbSettingsImplementation *parent,
                const string event, 
                const bool listen):mParent(*parent),mEvent(event),mListen(listen){

                }
            public:
                EventRegistrationJob() = delete;
                EventRegistrationJob(const EventRegistrationJob &) = delete;
                EventRegistrationJob &operator=(const EventRegistrationJob &) = delete;
                ~EventRegistrationJob()
                {
                }

                static Core::ProxyType<Core::IDispatch> Create(FbSettingsImplementation *parent,
                const string& event, const bool listen)
                {
                    return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<EventRegistrationJob>::Create(parent, event, listen)));
                }
                virtual void Dispatch()
                {
                    // Delegates (TTS/UserSettings/System) are routed centrally via SettingsDelegate
                    mParent.mDelegate->HandleAppEventNotifier(mEvent, mListen);
                }

            private:
            FbSettingsImplementation &mParent;
            const string mEvent;
            const bool mListen;

        };

    public:
        FbSettingsImplementation();
        ~FbSettingsImplementation();

        BEGIN_INTERFACE_MAP(FbSettingsImplementation)
        INTERFACE_ENTRY(Exchange::IFbSettings)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

        Core::hresult HandleAppEventNotifier(const string event, const bool listen, bool& status /* @out */) override;

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell) override;

        // The following public interfaces provide the org.rdk.System alias implementations via SystemDelegate.

        // PUBLIC_INTERFACE
        Core::hresult GetDeviceMake(string& make /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult GetDeviceName(string& name /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult SetDeviceName(const string name /* @in */) override;

        // PUBLIC_INTERFACE
        Core::hresult GetDeviceSku(string& sku /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult GetCountryCode(string& countryCode /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult SetCountryCode(const string countryCode /* @in */) override;

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnCountryCodeChanged(const bool listen /* @in */, bool& status /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult GetTimeZone(string& timeZone /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult SetTimeZone(const string timeZone /* @in */) override;

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnTimeZoneChanged(const bool listen /* @in */, bool& status /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult GetSecondScreenFriendlyName(string& name /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnFriendlyNameChanged(const bool listen /* @in */, bool& status /* @out */) override;

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnDeviceNameChanged(const bool listen /* @in */, bool& status /* @out */) override;

        // -------------------------
        // org.rdk.UserSettings aliases (35) - PUBLIC_INTERFACE methods
        // -------------------------

        // localization.language
        // PUBLIC_INTERFACE
        Core::hresult GetLanguage(string& language /* @out */);

        // localization.onLanguageChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnLanguageChanged(const bool listen /* @in */, bool& status /* @out */);

        // localization.locale
        // PUBLIC_INTERFACE
        Core::hresult GetLocale(string& locale /* @out */);

        // localization.setLocale
        // PUBLIC_INTERFACE
        Core::hresult SetLocale(const string locale /* @in */);

        // localization.onLocaleChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnLocaleChanged(const bool listen /* @in */, bool& status /* @out */);

        // localization.preferredAudioLanguages
        // PUBLIC_INTERFACE
        Core::hresult GetPreferredAudioLanguages(string& languages /* @out */);

        // localization.setPreferredAudioLanguages
        // PUBLIC_INTERFACE
        Core::hresult SetPreferredAudioLanguages(const string languages /* @in */);

        // localization.onPreferredAudioLanguagesChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnPreferredAudioLanguagesChanged(const bool listen /* @in */, bool& status /* @out */);

        // voiceguidance.setEnabled
        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceEnabled(const bool enabled /* @in */);

        // voiceguidance.setSpeed
        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceSpeed(const int speed /* @in */);

        // accessibility.onAudioDescriptionSettingsChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnAudioDescriptionSettingsChanged(const bool listen /* @in */, bool& status /* @out */);

        // accessibility.audioDescriptionSettings
        // PUBLIC_INTERFACE
        Core::hresult GetAudioDescriptionSettings(string& settingsJson /* @out */);

        // audiodescriptions.enabled
        // PUBLIC_INTERFACE
        Core::hresult GetAudioDescriptionsEnabled(bool& enabled /* @out */);

        // audiodescriptions.setEnabled
        // PUBLIC_INTERFACE
        Core::hresult SetAudioDescriptionsEnabled(const bool enabled /* @in */);

        // audiodescriptions.onEnabledChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnAudioDescriptionsEnabledChanged(const bool listen /* @in */, bool& status /* @out */);

        // accessibility.highContrastUI
        // PUBLIC_INTERFACE
        Core::hresult GetHighContrastUI(bool& enabled /* @out */);

        // accessibility.onHighContrastUIChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnHighContrastUIChanged(const bool listen /* @in */, bool& status /* @out */);

        // closedcaptions.enabled
        // PUBLIC_INTERFACE
        Core::hresult GetClosedCaptionsEnabled(bool& enabled /* @out */);

        // closedcaptions.setEnabled
        // PUBLIC_INTERFACE
        Core::hresult SetClosedCaptionsEnabled(const bool enabled /* @in */);

        // closedcaptions.onEnabledChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnClosedCaptionsEnabledChanged(const bool listen /* @in */, bool& status /* @out */);

        // closedcaptions.preferredLanguages
        // PUBLIC_INTERFACE
        Core::hresult GetClosedCaptionsPreferredLanguages(string& languages /* @out */);

        // closedcaptions.setPreferredLanguages
        // PUBLIC_INTERFACE
        Core::hresult SetClosedCaptionsPreferredLanguages(const string languages /* @in */);

        // closedaptions.onPreferredLanguagesChanged (intentional typo)
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnClosedaptionsPreferredLanguagesChanged(const bool listen /* @in */, bool& status /* @out */);

        // accessibility.onClosedCaptionsSettingsChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnClosedCaptionsSettingsChanged(const bool listen /* @in */, bool& status /* @out */);

        // voiceguidance.navigationHints
        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceNavigationHints(bool& enabled /* @out */);

        // voiceguidance.setNavigationHints
        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceNavigationHints(const bool enabled /* @in */);

        // voiceguidance.onNavigationHintsChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceNavigationHintsChanged(const bool listen /* @in */, bool& status /* @out */);

        // voiceguidance.rate
        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceRate(double& rate /* @out */);

        // voiceguidance.setRate
        // PUBLIC_INTERFACE
        Core::hresult SetVoiceGuidanceRate(const double rate /* @in */);

        // voiceguidance.onRateChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceRateChanged(const bool listen /* @in */, bool& status /* @out */);

        // voiceguidance.enabled
        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceEnabled(bool& enabled /* @out */);

        // voiceguidance.onEnabledChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceEnabledChanged(const bool listen /* @in */, bool& status /* @out */);

        // voiceguidance.speed
        // PUBLIC_INTERFACE
        Core::hresult GetVoiceGuidanceSpeed(int& speed /* @out */);

        // voiceguidance.onSpeedChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceSpeedChanged(const bool listen /* @in */, bool& status /* @out */);

        // accessibility.onVoiceGuidanceSettingsChanged
        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnVoiceGuidanceSettingsChanged(const bool listen /* @in */, bool& status /* @out */);

    private:
        PluginHost::IShell* mShell;
        std::shared_ptr<SettingsDelegate> mDelegate;
        std::shared_ptr<UserSettingsDelegate> mUserSettingsDelegate;
    };
}
}
