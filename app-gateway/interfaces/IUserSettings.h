#pragma once

#include <plugins/Module.h>

#ifndef ID_USERSETTINGS
#define ID_USERSETTINGS 0xFA100300
#endif

namespace WPEFramework {
namespace Exchange {

    // PUBLIC_INTERFACE
    struct EXTERNAL IUserSettings : virtual public Core::IUnknown {
        enum { ID = ID_USERSETTINGS };

        // Notification interface stub
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = ID_USERSETTINGS + 1 };
            virtual ~INotification() override = default;

            // Optional overrides (no-ops by default)
            virtual void OnAudioDescriptionChanged(const bool /*enabled*/) {}
            virtual void OnPreferredAudioLanguagesChanged(const string& /*preferredLanguages*/) {}
            virtual void OnPresentationLanguageChanged(const string& /*presentationLanguage*/) {}
            virtual void OnCaptionsChanged(const bool /*enabled*/) {}
            virtual void OnPreferredCaptionsLanguagesChanged(const string& /*preferredLanguages*/) {}
            virtual void OnPreferredClosedCaptionServiceChanged(const string& /*service*/) {}
            virtual void OnPrivacyModeChanged(const string& /*privacyMode*/) {}
            virtual void OnPinControlChanged(const bool /*pinControl*/) {}
            virtual void OnViewingRestrictionsChanged(const string& /*viewingRestrictions*/) {}
            virtual void OnViewingRestrictionsWindowChanged(const string& /*viewingRestrictionsWindow*/) {}
            virtual void OnLiveWatershedChanged(const bool /*liveWatershed*/) {}
            virtual void OnPlaybackWatershedChanged(const bool /*playbackWatershed*/) {}
            virtual void OnBlockNotRatedContentChanged(const bool /*blockNotRatedContent*/) {}
            virtual void OnPinOnPurchaseChanged(const bool /*pinOnPurchase*/) {}
            virtual void OnHighContrastChanged(const bool /*enabled*/) {}
            virtual void OnVoiceGuidanceChanged(const bool /*enabled*/) {}
            virtual void OnVoiceGuidanceRateChanged(const double /*rate*/) {}
            virtual void OnVoiceGuidanceHintsChanged(const bool /*hints*/) {}
            virtual void OnContentPinChanged(const string& /*contentPin*/) {}
        };

        // Registration for notifications.
        // PUBLIC_INTERFACE
        virtual uint32_t Register(INotification* /*notification*/) = 0;
        // PUBLIC_INTERFACE
        virtual uint32_t Unregister(INotification* /*notification*/) = 0;

        // Methods used in UserSettingsDelegate (stubs)
        // PUBLIC_INTERFACE
        virtual Core::hresult GetVoiceGuidance(bool& /*enabled*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult GetHighContrast(bool& /*enabled*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetVoiceGuidance(const bool /*enabled*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetVoiceGuidanceRate(const double /*rate*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetVoiceGuidanceHints(const bool /*enabled*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult GetVoiceGuidanceRate(double& /*rate*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult GetVoiceGuidanceHints(bool& /*enabled*/) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPresentationLanguage(string& /*presentationLanguage*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetPresentationLanguage(const string& /*presentationLanguage*/) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPreferredAudioLanguages(string& /*preferredLanguagesCsv*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetPreferredAudioLanguages(const string& /*preferredLanguagesCsv*/) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPreferredCaptionsLanguages(string& /*preferredLanguagesCsv*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetPreferredCaptionsLanguages(const string& /*preferredLanguagesCsv*/) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetAudioDescription(bool& /*enabled*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetAudioDescription(const bool /*enabled*/) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetCaptions(bool& /*enabled*/) = 0;
        // PUBLIC_INTERFACE
        virtual Core::hresult SetCaptions(const bool /*enabled*/) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
