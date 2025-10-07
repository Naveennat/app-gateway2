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
    };

} // namespace Exchange
} // namespace WPEFramework
