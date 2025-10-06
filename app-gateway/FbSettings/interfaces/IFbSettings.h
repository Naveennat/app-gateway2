#pragma once

#include <plugins/plugins.h>

namespace WPEFramework {
namespace Exchange {

    // Minimal IFbSettings interface required by FbSettingsImplementation and FbSettings.
    // This is a local stub, not the full upstream interface definition.
    struct EXTERNAL IFbSettings : virtual public Core::IUnknown {

        enum { ID = 0x0000FB01 }; // Local arbitrary ID; keep stable within this project.

        virtual ~IFbSettings() = default;

        // Event registration from app layer (Firebolt notifier bridge)
        virtual Core::hresult HandleAppEventNotifier(const string event,
                                                     const bool listen,
                                                     bool& status /* @out */) = 0;

        // SystemDelegate-backed aliases
        virtual Core::hresult GetDeviceMake(string& make /* @out */) = 0;
        virtual Core::hresult GetDeviceName(string& name /* @out */) = 0;
        virtual Core::hresult SetDeviceName(const string& name /* @in */) = 0;
        virtual Core::hresult GetDeviceSku(string& sku /* @out */) = 0;

        virtual Core::hresult GetCountryCode(string& countryCode /* @out */) = 0;
        virtual Core::hresult SetCountryCode(const string& countryCode /* @in */) = 0;
        virtual Core::hresult SubscribeOnCountryCodeChanged(const bool listen /* @in */, bool& status /* @out */) = 0;

        virtual Core::hresult GetTimeZone(string& timeZone /* @out */) = 0;
        virtual Core::hresult SetTimeZone(const string& timeZone /* @in */) = 0;
        virtual Core::hresult SubscribeOnTimeZoneChanged(const bool listen /* @in */, bool& status /* @out */) = 0;

        virtual Core::hresult GetSecondScreenFriendlyName(string& name /* @out */) = 0;
        virtual Core::hresult SubscribeOnFriendlyNameChanged(const bool listen /* @in */, bool& status /* @out */) = 0;
        virtual Core::hresult SubscribeOnDeviceNameChanged(const bool listen /* @in */, bool& status /* @out */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
