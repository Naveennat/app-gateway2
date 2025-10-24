#pragma once

/*
 * Minimal stub for the Exchange::IUserSettings interface to satisfy build-time includes.
 * Provides the methods and nested INotification interface used by FbSettings UserSettingsDelegate.
 */

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IUserSettings : virtual public Core::IUnknown {
        // Placeholder interface ID; in a full Thunder environment, this would be defined in Ids.h
        enum { ID = 0xFA00000A };

        virtual ~IUserSettings() = default;

        // Notification sink interface
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = 0xFA00000B };
            virtual ~INotification() = default;

            // PUBLIC_INTERFACE
            virtual void OnAudioDescriptionChanged(const bool enabled) = 0;
            /** Notify when audio description setting changes. */

            // PUBLIC_INTERFACE
            virtual void OnPreferredAudioLanguagesChanged(const string& preferredLanguages) = 0;
            /** Notify when preferred audio languages change (comma-separated). */

            // PUBLIC_INTERFACE
            virtual void OnPresentationLanguageChanged(const string& presentationLanguage) = 0;
            /** Notify when presentation language (locale) changes, e.g. "en-US". */

            // PUBLIC_INTERFACE
            virtual void OnCaptionsChanged(const bool enabled) = 0;
            /** Notify when captions enabled state changes. */

            // PUBLIC_INTERFACE
            virtual void OnPreferredCaptionsLanguagesChanged(const string& preferredLanguages) = 0;
            /** Notify when preferred captions languages change (comma-separated). */

            // PUBLIC_INTERFACE
            virtual void OnPreferredClosedCaptionServiceChanged(const string& service) = 0;

            // PUBLIC_INTERFACE
            virtual void OnPrivacyModeChanged(const string& privacyMode) = 0;

            // PUBLIC_INTERFACE
            virtual void OnPinControlChanged(const bool pinControl) = 0;

            // PUBLIC_INTERFACE
            virtual void OnViewingRestrictionsChanged(const string& viewingRestrictions) = 0;

            // PUBLIC_INTERFACE
            virtual void OnViewingRestrictionsWindowChanged(const string& viewingRestrictionsWindow) = 0;

            // PUBLIC_INTERFACE
            virtual void OnLiveWatershedChanged(const bool liveWatershed) = 0;

            // PUBLIC_INTERFACE
            virtual void OnPlaybackWatershedChanged(const bool playbackWatershed) = 0;

            // PUBLIC_INTERFACE
            virtual void OnBlockNotRatedContentChanged(const bool blockNotRatedContent) = 0;

            // PUBLIC_INTERFACE
            virtual void OnPinOnPurchaseChanged(const bool pinOnPurchase) = 0;

            // PUBLIC_INTERFACE
            virtual void OnHighContrastChanged(const bool enabled) = 0;
            /** Notify when high contrast UI setting changes. */

            // PUBLIC_INTERFACE
            virtual void OnVoiceGuidanceChanged(const bool enabled) = 0;
            /** Notify when voice guidance enabled state changes. */

            // PUBLIC_INTERFACE
            virtual void OnVoiceGuidanceRateChanged(const double rate) = 0;
            /** Notify when voice guidance rate changes. */

            // PUBLIC_INTERFACE
            virtual void OnVoiceGuidanceHintsChanged(const bool hints) = 0;
            /** Notify when voice guidance navigation hints change. */

            // PUBLIC_INTERFACE
            virtual void OnContentPinChanged(const string& contentPin) = 0;
            /** Notify when content pin changes. */
        };

        // PUBLIC_INTERFACE
        virtual uint32_t Register(INotification* notification /* @in */) = 0;
        /** Register for IUserSettings notifications. */

        // PUBLIC_INTERFACE
        virtual uint32_t Unregister(INotification* notification /* @in */) = 0;
        /** Unregister for IUserSettings notifications. */

        // Getter methods
        // PUBLIC_INTERFACE
        virtual Core::hresult GetVoiceGuidance(bool& enabled /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetAudioDescription(bool& enabled /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetHighContrast(bool& enabled /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetCaptions(bool& enabled /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetVoiceGuidanceRate(double& rate /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetVoiceGuidanceHints(bool& hints /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPresentationLanguage(string& presentationLanguage /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPreferredAudioLanguages(string& audioLanguages /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPreferredCaptionsLanguages(string& captionsLanguages /* @out */) = 0;

        // Setter methods
        // PUBLIC_INTERFACE
        virtual Core::hresult SetVoiceGuidance(const bool enabled /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetAudioDescription(const bool enabled /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetCaptions(const bool enabled /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetVoiceGuidanceRate(const double rate /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetVoiceGuidanceHints(const bool enabled /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetPresentationLanguage(const string& locale /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetPreferredAudioLanguages(const string& commaSeparatedLanguages /* @in */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult SetPreferredCaptionsLanguages(const string& commaSeparatedLanguages /* @in */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
