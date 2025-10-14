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
#include "FbSettingsImplementation.h"
#include "UtilsLogging.h"
#include "delegate/SettingsDelegate.h"
#include "delegate/SystemDelegate.h"

namespace WPEFramework
{
    namespace Plugin
    {

        SERVICE_REGISTRATION(FbSettingsImplementation, 1, 0);

        FbSettingsImplementation::FbSettingsImplementation() : 
        mShell(nullptr)   
        {
            LOGINFO("FbSettingsImplementation::FbSettingsImplementation() called");
            mDelegate = std::make_shared<SettingsDelegate>();
        }

        FbSettingsImplementation::~FbSettingsImplementation()
        {
            LOGINFO("FbSettingsImplementation::~FbSettingsImplementation() called");
            // Cleanup resources if needed
            if (mShell != nullptr)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        Core::hresult FbSettingsImplementation::HandleAppEventNotifier(const string event /* @in */,
                                    const bool listen /* @in */,
                                    bool &status /* @out */) {
            LOGINFO("HandleFireboltNotifier [event=%s listen=%s]",
                    event.c_str(), listen ? "true" : "false");
            status = true;
            Core::IWorkerPool::Instance().Submit(EventRegistrationJob::Create(this, event, listen));
            return Core::ERROR_NONE;
        }

        // Delegated alias methods

        Core::hresult FbSettingsImplementation::GetDeviceMake(string& make) {
            LOGINFO("FbSettingsImplementation::GetDeviceMake(make) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->GetDeviceMake(make);
        }

        Core::hresult FbSettingsImplementation::GetDeviceName(string& name) {
            LOGINFO("FbSettingsImplementation::GetDeviceName(name) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->GetDeviceName(name);
        }

        Core::hresult FbSettingsImplementation::SetDeviceName(const string name) {
            LOGINFO("FbSettingsImplementation::SetDeviceName(name) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->SetDeviceName(name);
        }

        Core::hresult FbSettingsImplementation::GetDeviceSku(string& sku) {
            LOGINFO("FbSettingsImplementation::GetDeviceSku(sku) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->GetDeviceSku(sku);
        }

        Core::hresult FbSettingsImplementation::GetCountryCode(string& countryCode) {
            LOGINFO("FbSettingsImplementation::GetCountryCode(countryCode) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->GetCountryCode(countryCode);
        }

        Core::hresult FbSettingsImplementation::SetCountryCode(const string countryCode) {
            LOGINFO("FbSettingsImplementation::SetCountryCode(countryCode) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->SetCountryCode(countryCode);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnCountryCodeChanged(const bool listen, bool& status) {
            LOGINFO("FbSettingsImplementation::SubscribeOnCountryCodeChanged(listen, status) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->SubscribeOnCountryCodeChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetTimeZone(string& timeZone) {
            LOGINFO("FbSettingsImplementation::GetTimeZone(timeZone) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->GetTimeZone(timeZone);
        }

        Core::hresult FbSettingsImplementation::SetTimeZone(const string timeZone) {
            LOGINFO("FbSettingsImplementation::SetTimeZone(timeZone) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->SetTimeZone(timeZone);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnTimeZoneChanged(const bool listen, bool& status) {
            LOGINFO("FbSettingsImplementation::SubscribeOnTimeZoneChanged(listen, status) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->SubscribeOnTimeZoneChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetSecondScreenFriendlyName(string& name) {
            LOGINFO("FbSettingsImplementation::GetSecondScreenFriendlyName(name) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->GetSecondScreenFriendlyName(name);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnFriendlyNameChanged(const bool listen, bool& status) {
            LOGINFO("FbSettingsImplementation::SubscribeOnFriendlyNameChanged(listen, status) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            return sys->SubscribeOnFriendlyNameChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnDeviceNameChanged(const bool listen, bool& status) {
            LOGINFO("FbSettingsImplementation::SubscribeOnDeviceNameChanged(listen, status) called");
            auto sys = (mDelegate ? mDelegate->getSystemDelegate() : nullptr);
            if (!sys) return Core::ERROR_UNAVAILABLE;
            // Same underlying event as friendlyName changed
            return sys->SubscribeOnFriendlyNameChanged(listen, status);
        }

        // ------------- UserSettings forwarders -------------

        Core::hresult FbSettingsImplementation::GetLanguage(string& language) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetLanguage(language);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnLanguageChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnLanguageChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetLocale(string& locale) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetLocale(locale);
        }

        Core::hresult FbSettingsImplementation::SetLocale(const string locale) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetLocale(locale);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnLocaleChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnLocaleChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetPreferredAudioLanguages(string& languages) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetPreferredAudioLanguages(languages);
        }

        Core::hresult FbSettingsImplementation::SetPreferredAudioLanguages(const string languages) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetPreferredAudioLanguages(languages);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnPreferredAudioLanguagesChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnPreferredAudioLanguagesChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::SetVoiceGuidanceEnabled(const bool enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetVoiceGuidanceEnabled(enabled);
        }

        Core::hresult FbSettingsImplementation::SetVoiceGuidanceSpeed(const int speed) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetVoiceGuidanceSpeed(speed);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnAudioDescriptionSettingsChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnAudioDescriptionSettingsChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetAudioDescriptionSettings(string& settingsJson) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetAudioDescriptionSettings(settingsJson);
        }

        Core::hresult FbSettingsImplementation::GetAudioDescriptionsEnabled(bool& enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetAudioDescriptionsEnabled(enabled);
        }

        Core::hresult FbSettingsImplementation::SetAudioDescriptionsEnabled(const bool enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetAudioDescriptionsEnabled(enabled);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnAudioDescriptionsEnabledChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnAudioDescriptionsEnabledChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetHighContrastUI(bool& enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetHighContrastUI(enabled);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnHighContrastUIChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnHighContrastUIChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetClosedCaptionsEnabled(bool& enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetClosedCaptionsEnabled(enabled);
        }

        Core::hresult FbSettingsImplementation::SetClosedCaptionsEnabled(const bool enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetClosedCaptionsEnabled(enabled);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnClosedCaptionsEnabledChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnClosedCaptionsEnabledChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetClosedCaptionsPreferredLanguages(string& languages) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetClosedCaptionsPreferredLanguages(languages);
        }

        Core::hresult FbSettingsImplementation::SetClosedCaptionsPreferredLanguages(const string languages) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetClosedCaptionsPreferredLanguages(languages);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnClosedaptionsPreferredLanguagesChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnClosedaptionsPreferredLanguagesChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnClosedCaptionsSettingsChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnClosedCaptionsSettingsChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetVoiceGuidanceNavigationHints(bool& enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetVoiceGuidanceNavigationHints(enabled);
        }

        Core::hresult FbSettingsImplementation::SetVoiceGuidanceNavigationHints(const bool enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetVoiceGuidanceNavigationHints(enabled);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnVoiceGuidanceNavigationHintsChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnVoiceGuidanceNavigationHintsChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetVoiceGuidanceRate(double& rate) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetVoiceGuidanceRate(rate);
        }

        Core::hresult FbSettingsImplementation::SetVoiceGuidanceRate(const double rate) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SetVoiceGuidanceRate(rate);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnVoiceGuidanceRateChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnVoiceGuidanceRateChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetVoiceGuidanceEnabled(bool& enabled) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetVoiceGuidanceEnabled(enabled);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnVoiceGuidanceEnabledChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnVoiceGuidanceEnabledChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::GetVoiceGuidanceSpeed(int& speed) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->GetVoiceGuidanceSpeed(speed);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnVoiceGuidanceSpeedChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnVoiceGuidanceSpeedChanged(listen, status);
        }

        Core::hresult FbSettingsImplementation::SubscribeOnVoiceGuidanceSettingsChanged(const bool listen, bool& status) {
            auto d = mUserSettingsDelegate;
            if (!d) return Core::ERROR_UNAVAILABLE;
            return d->SubscribeOnVoiceGuidanceSettingsChanged(listen, status);
        }

        uint32_t FbSettingsImplementation::Configure(PluginHost::IShell *shell)
        {
            LOGINFO("Configuring FbSettings");
            uint32_t result = Core::ERROR_NONE;
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();
            mDelegate->setShell(mShell);
            // Also initialize a direct UserSettings delegate for call forwarding
            if (!mUserSettingsDelegate) {
                // Pass nullptr for AppNotifications, registration still handled via central SettingsDelegate
                mUserSettingsDelegate = std::make_shared<UserSettingsDelegate>(mShell, nullptr);
            }
            return result;
        }
    }
}
