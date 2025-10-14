#pragma once

/*
 * Minimal stub for the Exchange::IFbSettings interface to satisfy build-time includes.
 * This header defines the interface used by FbSettings and FbSettingsImplementation.
 * Note: The ID value is a placeholder for compilation purposes only.
 */

// NOTE: Method declaration conventions for this interface:
// - Return type: Core::hresult
// - Input scalars/strings are passed by const value (e.g., const string, const bool)
// - Output parameters are passed by non-const reference and annotated with /* @out */
// - Precede each method declaration with structured documentation tags:
//     // @text <lowerCamelCase method name>
//     // @brief <one-line description>
//     // @param <name>: <description>
//     // @returns Core::hresult

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IFbSettings : virtual public Core::IUnknown {
        // Placeholder interface ID; in a full Thunder environment, this would be defined in Ids.h
        enum { ID = 0xFA000001 };

        virtual ~IFbSettings() = default;

        // @text handleAppEventNotifier
        // @brief Register or unregister interest for the given application event.
        // @param event: Name of the application event to register or unregister.
        // @param listen: True to subscribe (register), false to unsubscribe.
        // @param status: Operation result indicating whether the action was applied.
        // @returns Core::hresult
        virtual Core::hresult HandleAppEventNotifier(const string event, const bool listen, bool& status /* @out */) = 0;

        // -------------------------
        // org.rdk.System aliases
        // -------------------------

        // @text getDeviceMake
        // @brief Retrieve the device manufacturer or make.
        // @param make: Output string receiving the device make.
        // @returns Core::hresult
        virtual Core::hresult GetDeviceMake(string& make /* @out */) = 0;

        // @text getDeviceName
        // @brief Retrieve the device friendly name.
        // @param name: Output string receiving the device friendly name.
        // @returns Core::hresult
        virtual Core::hresult GetDeviceName(string& name /* @out */) = 0;

        // @text setDeviceName
        // @brief Set the device friendly name.
        // @param name: New device friendly name.
        // @returns Core::hresult
        virtual Core::hresult SetDeviceName(const string name) = 0;

        // @text getDeviceSku
        // @brief Retrieve the device SKU.
        // @param sku: Output string receiving the device SKU.
        // @returns Core::hresult
        virtual Core::hresult GetDeviceSku(string& sku /* @out */) = 0;

        // @text getCountryCode
        // @brief Retrieve the device country code.
        // @param countryCode: Output string receiving the country code.
        // @returns Core::hresult
        virtual Core::hresult GetCountryCode(string& countryCode /* @out */) = 0;

        // @text setCountryCode
        // @brief Set the device country code.
        // @param countryCode: New device country code.
        // @returns Core::hresult
        virtual Core::hresult SetCountryCode(const string countryCode) = 0;

        // @text subscribeOnCountryCodeChanged
        // @brief Subscribe or unsubscribe to country code change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating whether the subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnCountryCodeChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getTimeZone
        // @brief Retrieve the device time zone identifier.
        // @param timeZone: Output string receiving the time zone.
        // @returns Core::hresult
        virtual Core::hresult GetTimeZone(string& timeZone /* @out */) = 0;

        // @text setTimeZone
        // @brief Set the device time zone identifier.
        // @param timeZone: New device time zone.
        // @returns Core::hresult
        virtual Core::hresult SetTimeZone(const string timeZone) = 0;

        // @text subscribeOnTimeZoneChanged
        // @brief Subscribe or unsubscribe to time zone change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating whether the subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnTimeZoneChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getSecondScreenFriendlyName
        // @brief Retrieve the second screen friendly name.
        // @param name: Output string receiving the second screen friendly name.
        // @returns Core::hresult
        virtual Core::hresult GetSecondScreenFriendlyName(string& name /* @out */) = 0;

        // @text subscribeOnFriendlyNameChanged
        // @brief Subscribe or unsubscribe to friendly name change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating whether the subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnFriendlyNameChanged(const bool listen, bool& status /* @out */) = 0;

        // @text subscribeOnDeviceNameChanged
        // @brief Subscribe or unsubscribe to device name change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating whether the subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnDeviceNameChanged(const bool listen, bool& status /* @out */) = 0;

        // -------------------------
        // org.rdk.UserSettings aliases (35)
        // -------------------------

        // @text getLanguage
        // @brief Retrieve the current UI language (e.g., "en") derived from locale.
        // @param language: Output ISO language subtag (lowercase), e.g., "en".
        // @returns Core::hresult
        virtual Core::hresult GetLanguage(string& language /* @out */) = 0;

        // @text subscribeOnLanguageChanged
        // @brief Subscribe or unsubscribe to language change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnLanguageChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getLocale
        // @brief Retrieve the current UI locale (e.g., "en-US").
        // @param locale: Output locale string.
        // @returns Core::hresult
        virtual Core::hresult GetLocale(string& locale /* @out */) = 0;

        // @text setLocale
        // @brief Set the UI locale (e.g., "en-US").
        // @param locale: New locale string.
        // @returns Core::hresult
        virtual Core::hresult SetLocale(const string locale) = 0;

        // @text subscribeOnLocaleChanged
        // @brief Subscribe or unsubscribe to locale change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnLocaleChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getPreferredAudioLanguages
        // @brief Retrieve the preferred audio languages (comma-separated or JSON as implemented).
        // @param languages: Output preferred audio languages.
        // @returns Core::hresult
        virtual Core::hresult GetPreferredAudioLanguages(string& languages /* @out */) = 0;

        // @text setPreferredAudioLanguages
        // @brief Set the preferred audio languages.
        // @param languages: New preferred audio languages.
        // @returns Core::hresult
        virtual Core::hresult SetPreferredAudioLanguages(const string languages) = 0;

        // @text subscribeOnPreferredAudioLanguagesChanged
        // @brief Subscribe or unsubscribe to preferred audio languages change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnPreferredAudioLanguagesChanged(const bool listen, bool& status /* @out */) = 0;

        // @text setVoiceGuidanceEnabled
        // @brief Enable or disable voice guidance (TTS).
        // @param enabled: True to enable, false to disable.
        // @returns Core::hresult
        virtual Core::hresult SetVoiceGuidanceEnabled(const bool enabled) = 0;

        // @text setVoiceGuidanceSpeed
        // @brief Set the voice guidance speed (implementation-defined integer scale).
        // @param speed: Speed value (implementation-defined).
        // @returns Core::hresult
        virtual Core::hresult SetVoiceGuidanceSpeed(const int speed) = 0;

        // @text subscribeOnAudioDescriptionSettingsChanged
        // @brief Subscribe or unsubscribe to audio description settings change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnAudioDescriptionSettingsChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getAudioDescriptionSettings
        // @brief Retrieve audio description settings as a JSON string.
        // @param settingsJson: Output JSON string with audio description settings.
        // @returns Core::hresult
        virtual Core::hresult GetAudioDescriptionSettings(string& settingsJson /* @out */) = 0;

        // @text getAudioDescriptionsEnabled
        // @brief Retrieve whether audio descriptions are enabled.
        // @param enabled: Output flag set to true if enabled.
        // @returns Core::hresult
        virtual Core::hresult GetAudioDescriptionsEnabled(bool& enabled /* @out */) = 0;

        // @text setAudioDescriptionsEnabled
        // @brief Enable or disable audio descriptions.
        // @param enabled: True to enable, false to disable.
        // @returns Core::hresult
        virtual Core::hresult SetAudioDescriptionsEnabled(const bool enabled) = 0;

        // @text subscribeOnAudioDescriptionsEnabledChanged
        // @brief Subscribe or unsubscribe to audio descriptions enabled change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnAudioDescriptionsEnabledChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getHighContrastUI
        // @brief Retrieve whether high contrast UI is enabled.
        // @param enabled: Output flag set to true if high contrast UI is enabled.
        // @returns Core::hresult
        virtual Core::hresult GetHighContrastUI(bool& enabled /* @out */) = 0;

        // @text subscribeOnHighContrastUIChanged
        // @brief Subscribe or unsubscribe to high contrast UI change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnHighContrastUIChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getClosedCaptionsEnabled
        // @brief Retrieve whether closed captions are enabled.
        // @param enabled: Output flag set to true if enabled.
        // @returns Core::hresult
        virtual Core::hresult GetClosedCaptionsEnabled(bool& enabled /* @out */) = 0;

        // @text setClosedCaptionsEnabled
        // @brief Enable or disable closed captions.
        // @param enabled: True to enable, false to disable.
        // @returns Core::hresult
        virtual Core::hresult SetClosedCaptionsEnabled(const bool enabled) = 0;

        // @text subscribeOnClosedCaptionsEnabledChanged
        // @brief Subscribe or unsubscribe to closed captions enabled change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnClosedCaptionsEnabledChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getClosedCaptionsPreferredLanguages
        // @brief Retrieve preferred closed captions languages.
        // @param languages: Output preferred languages string.
        // @returns Core::hresult
        virtual Core::hresult GetClosedCaptionsPreferredLanguages(string& languages /* @out */) = 0;

        // @text setClosedCaptionsPreferredLanguages
        // @brief Set preferred closed captions languages.
        // @param languages: New preferred languages string.
        // @returns Core::hresult
        virtual Core::hresult SetClosedCaptionsPreferredLanguages(const string languages) = 0;

        // @text subscribeOnClosedaptionsPreferredLanguagesChanged
        // @brief Subscribe or unsubscribe to preferred closed captions languages changed notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        // NOTE: The method name intentionally preserves the original alias typo "closedaptions".
        virtual Core::hresult SubscribeOnClosedaptionsPreferredLanguagesChanged(const bool listen, bool& status /* @out */) = 0;

        // @text subscribeOnClosedCaptionsSettingsChanged
        // @brief Subscribe or unsubscribe to closed captions settings change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnClosedCaptionsSettingsChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getVoiceGuidanceNavigationHints
        // @brief Retrieve whether voice guidance navigation hints are enabled.
        // @param enabled: Output flag set to true if enabled.
        // @returns Core::hresult
        virtual Core::hresult GetVoiceGuidanceNavigationHints(bool& enabled /* @out */) = 0;

        // @text setVoiceGuidanceNavigationHints
        // @brief Enable or disable voice guidance navigation hints.
        // @param enabled: True to enable, false to disable.
        // @returns Core::hresult
        virtual Core::hresult SetVoiceGuidanceNavigationHints(const bool enabled) = 0;

        // @text subscribeOnVoiceGuidanceNavigationHintsChanged
        // @brief Subscribe or unsubscribe to voice guidance navigation hints change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnVoiceGuidanceNavigationHintsChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getVoiceGuidanceRate
        // @brief Retrieve the voice guidance rate as a double (implementation-defined scale).
        // @param rate: Output rate value.
        // @returns Core::hresult
        virtual Core::hresult GetVoiceGuidanceRate(double& rate /* @out */) = 0;

        // @text setVoiceGuidanceRate
        // @brief Set the voice guidance rate as a double (implementation-defined scale).
        // @param rate: Rate value to set.
        // @returns Core::hresult
        virtual Core::hresult SetVoiceGuidanceRate(const double rate) = 0;

        // @text subscribeOnVoiceGuidanceRateChanged
        // @brief Subscribe or unsubscribe to voice guidance rate change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnVoiceGuidanceRateChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getVoiceGuidanceEnabled
        // @brief Retrieve whether voice guidance (TTS) is enabled.
        // @param enabled: Output flag set to true if enabled.
        // @returns Core::hresult
        virtual Core::hresult GetVoiceGuidanceEnabled(bool& enabled /* @out */) = 0;

        // @text subscribeOnVoiceGuidanceEnabledChanged
        // @brief Subscribe or unsubscribe to voice guidance enabled change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnVoiceGuidanceEnabledChanged(const bool listen, bool& status /* @out */) = 0;

        // @text getVoiceGuidanceSpeed
        // @brief Retrieve the voice guidance speed as an integer (implementation-defined).
        // @param speed: Output speed value.
        // @returns Core::hresult
        virtual Core::hresult GetVoiceGuidanceSpeed(int& speed /* @out */) = 0;

        // @text subscribeOnVoiceGuidanceSpeedChanged
        // @brief Subscribe or unsubscribe to voice guidance speed change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnVoiceGuidanceSpeedChanged(const bool listen, bool& status /* @out */) = 0;

        // @text subscribeOnVoiceGuidanceSettingsChanged
        // @brief Subscribe or unsubscribe to voice guidance settings change notifications.
        // @param listen: True to subscribe, false to unsubscribe.
        // @param status: Operation result indicating if subscription state was applied.
        // @returns Core::hresult
        virtual Core::hresult SubscribeOnVoiceGuidanceSettingsChanged(const bool listen, bool& status /* @out */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
