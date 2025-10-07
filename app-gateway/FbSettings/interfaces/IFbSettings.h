#pragma once

/*
 * Minimal stub for the Exchange::IFbSettings interface to satisfy build-time includes.
 * This header defines the interface used by FbSettings and FbSettingsImplementation.
 * Note: The ID value is a placeholder for compilation purposes only.
 */

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IFbSettings : virtual public Core::IUnknown {
        // Placeholder interface ID; in a full Thunder environment, this would be defined in Ids.h
        enum { ID = 0xFA000001 };

        virtual ~IFbSettings() = default;

        // @text handleAppEventNotifier
        // @brief Handle AppEvent Notfier expectations for a given event
        // @param event: the event for registration
        // @param listen: whether to listen
        // @param status: status to be filled in
        // @returns Core::hresult
        virtual Core::hresult HandleAppEventNotifier(const string event, const bool listen, bool& status /* @out */) = 0;

        // PUBLIC_INTERFACE
        virtual Core::hresult GetDeviceMake(string& make /* @out */) = 0;
        /** Retrieve device make. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetDeviceName(string& name /* @out */) = 0;
        /** Retrieve device friendly name. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SetDeviceName(const string& name /* @in */) = 0;
        /** Set device friendly name. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetDeviceSku(string& sku /* @out */) = 0;
        /** Retrieve device SKU. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetCountryCode(string& countryCode /* @out */) = 0;
        /** Retrieve device country code. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SetCountryCode(const string& countryCode /* @in */) = 0;
        /** Set device country code. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SubscribeOnCountryCodeChanged(const bool listen /* @in */,
                                                            bool& status /* @out */) = 0;
        /** Subscribe/unsubscribe to country code change notifications. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetTimeZone(string& timeZone /* @out */) = 0;
        /** Retrieve device time zone. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SetTimeZone(const string& timeZone /* @in */) = 0;
        /** Set device time zone. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SubscribeOnTimeZoneChanged(const bool listen /* @in */,
                                                         bool& status /* @out */) = 0;
        /** Subscribe/unsubscribe to time zone change notifications. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetSecondScreenFriendlyName(string& name /* @out */) = 0;
        /** Retrieve second screen friendly name. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SubscribeOnFriendlyNameChanged(const bool listen /* @in */,
                                                             bool& status /* @out */) = 0;
        /** Subscribe/unsubscribe to friendly name change notifications. */

        // PUBLIC_INTERFACE
        virtual Core::hresult SubscribeOnDeviceNameChanged(const bool listen /* @in */,
                                                           bool& status /* @out */) = 0;
        /** Subscribe/unsubscribe to device name change notifications. */
    };

} // namespace Exchange
} // namespace WPEFramework
