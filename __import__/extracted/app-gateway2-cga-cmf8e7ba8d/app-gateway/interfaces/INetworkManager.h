#pragma once

/* Minimal stub of INetworkManager to satisfy compilation.
 * This does not implement real behavior; it only provides
 * the types and signatures referenced by NetworkDelegate.
 */

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL INetworkManager : virtual public Core::IUnknown {
        // Placeholder ID
        enum { ID = 0xFA100100 };

        // Enumerations required by NetworkDelegate
        enum InterfaceType {
            INTERFACE_TYPE_UNKNOWN = 0,
            INTERFACE_TYPE_ETHERNET = 1,
            INTERFACE_TYPE_WIFI = 2
        };

        enum InterfaceState {
            INTERFACE_STATE_UNKNOWN = 0,
            INTERFACE_STATE_UP = 1,
            INTERFACE_STATE_DOWN = 2
        };

        enum IPStatus {
            IPSTATUS_UNKNOWN = 0,
            IPSTATUS_ACQUIRED = 1,
            IPSTATUS_LOST = 2
        };

        enum WiFiState {
            WIFI_UNKNOWN = 0,
            WIFI_ENABLED = 1,
            WIFI_DISABLED = 2
        };

        enum WiFiSignalQuality {
            WIFI_SIGNAL_UNKNOWN = 0,
            WIFI_SIGNAL_POOR = 1,
            WIFI_SIGNAL_FAIR = 2,
            WIFI_SIGNAL_GOOD = 3,
            WIFI_SIGNAL_EXCELLENT = 4
        };

        enum InternetStatus {
            INTERNET_UNKNOWN = 0,
            INTERNET_FULLY_CONNECTED = 1,
            INTERNET_CAPTIVE_PORTAL = 2,
            INTERNET_LIMITED = 3,
            INTERNET_NOT_AVAILABLE = 4
        };

        struct InterfaceDetails {
            bool connected { false };
            InterfaceType type { INTERFACE_TYPE_UNKNOWN };
            Core::JSON::String interfaceName;
        };

        // Iterator interface stub
        struct EXTERNAL IInterfaceDetailsIterator : virtual public Core::IUnknown {
            enum { ID = 0xFA100101 };
            virtual ~IInterfaceDetailsIterator() override = default;

            // PUBLIC_INTERFACE
            virtual bool Next(InterfaceDetails& iface /* @out */) = 0;
            /** Retrieve next interface details if available. */
        };

        // Notification interface stub
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = 0xFA100102 };
            virtual ~INotification() override = default;

            // Optional overrides
            virtual void onInterfaceStateChange(const InterfaceState /*state*/, const string /*interface*/) {}
            virtual void onActiveInterfaceChange(const string /*prevActiveInterface*/, const string /*currentActiveInterface*/) {}
            virtual void onIPAddressChange(const string /*interface*/, const string /*ipversion*/, const string /*ipaddress*/, const IPStatus /*status*/) {}
            virtual void onInternetStatusChange(const InternetStatus /*prevState*/, const InternetStatus /*currState*/, const string /*interface*/) {}
            virtual void onAvailableSSIDs(const string /*jsonOfScanResults*/) {}
            virtual void onWiFiStateChange(const WiFiState /*state*/) {}
            virtual void onWiFiSignalQualityChange(const string /*ssid*/, const string /*strength*/, const string /*noise*/, const string /*snr*/, const WiFiSignalQuality /*quality*/) {}
        };

        // PUBLIC_INTERFACE
        virtual uint32_t GetAvailableInterfaces(IInterfaceDetailsIterator*& iterator /* @out */) = 0;
        /** Populate iterator with available interfaces. */

        // PUBLIC_INTERFACE
        virtual uint32_t Register(INotification* /*notification*/) = 0;
        /** Register for notifications. */

        // PUBLIC_INTERFACE
        virtual uint32_t Unregister(INotification* /*notification*/) = 0;
        /** Unregister notifications. */
    };

} // namespace Exchange
} // namespace WPEFramework
