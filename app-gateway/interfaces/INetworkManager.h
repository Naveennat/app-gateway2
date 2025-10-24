#pragma once

/*
 * Expanded minimal stub for the Exchange::INetworkManager interface.
 * This version defines the nested types, iterators and notification interface
 * used by FbSettings/delegate/NetworkDelegate.h, while keeping a lightweight
 * contract to satisfy compilation in this repository without Thunder headers.
 */

#include <plugins/Module.h>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL INetworkManager : virtual public Core::IUnknown {
        // Placeholder ID; real value would be provided by Thunder Interfaces.
        enum { ID = 0xFA000007 };
        virtual ~INetworkManager() = default;

        // Interface type enums used by NetworkDelegate.
        enum InterfaceType : uint8_t {
            INTERFACE_TYPE_UNKNOWN = 0,
            INTERFACE_TYPE_ETHERNET = 1,
            INTERFACE_TYPE_WIFI = 2
        };

        enum InterfaceState : uint8_t {
            IFACE_STATE_UNKNOWN = 0,
            IFACE_STATE_UP = 1,
            IFACE_STATE_DOWN = 2
        };

        enum IPStatus : uint8_t {
            IPSTATUS_UNKNOWN = 0,
            IPSTATUS_ACQUIRED = 1,
            IPSTATUS_LOST = 2
        };

        enum InternetStatus : uint8_t {
            INTERNET_FULLY_CONNECTED = 0,
            INTERNET_CAPTIVE_PORTAL = 1,
            INTERNET_LIMITED = 2,
            INTERNET_NOT_AVAILABLE = 3
        };

        enum WiFiState : uint8_t {
            WIFI_STATE_UNKNOWN = 0
        };

        enum WiFiSignalQuality : uint8_t {
            WIFI_SIGNAL_QUALITY_UNKNOWN = 0
        };

        // Basic interface details used by iterator and NetworkDelegate.
        struct InterfaceDetails {
            InterfaceType type { INTERFACE_TYPE_UNKNOWN };
            bool connected { false };
        };

        // Iterator for interface details list; minimal stub.
        struct EXTERNAL IInterfaceDetailsIterator : virtual public Core::IUnknown {
            enum { ID = 0xFA000008 };
            virtual ~IInterfaceDetailsIterator() = default;

            // PUBLIC_INTERFACE
            virtual bool Next(InterfaceDetails& iface /* @out */) = 0;
            /** Move to next interface; returns false when exhausted. */

            // PUBLIC_INTERFACE
            virtual void Reset() = 0;
            /** Reset iteration to beginning. */
        };

        // Notification interface used by NetworkDelegate.
        struct EXTERNAL INotification : virtual public Core::IUnknown {
            enum { ID = 0xFA000009 };
            virtual ~INotification() = default;

            // PUBLIC_INTERFACE
            virtual void onInterfaceStateChange(const InterfaceState /*state*/, const string /*interface*/) = 0;
            /** Notifies interface state changes. */

            // PUBLIC_INTERFACE
            virtual void onActiveInterfaceChange(const string /*prev*/, const string /*current*/) = 0;
            /** Notifies active interface changes. */

            // PUBLIC_INTERFACE
            virtual void onIPAddressChange(const string /*interface*/, const string /*ipversion*/,
                                           const string /*ipaddress*/, const IPStatus /*status*/) = 0;
            /** Notifies IP address change. */

            // PUBLIC_INTERFACE
            virtual void onInternetStatusChange(const InternetStatus /*prevState*/, const InternetStatus /*currState*/) = 0;
            /** Notifies internet connectivity status changes. */

            // PUBLIC_INTERFACE
            virtual void onAvailableSSIDs(const string /*jsonOfScanResults*/) = 0;
            /** Notifies available SSIDs list. */

            // PUBLIC_INTERFACE
            virtual void onWiFiStateChange(const WiFiState /*state*/) = 0;
            /** Notifies WiFi state changes. */

            // PUBLIC_INTERFACE
            virtual void onWiFiSignalQualityChange(const string /*ssid*/, const string /*strength*/,
                                                   const string /*noise*/, const string /*snr*/,
                                                   const WiFiSignalQuality /*quality*/) = 0;
            /** Notifies WiFi signal quality change. */
        };

        // PUBLIC_INTERFACE
        virtual uint32_t Register(INotification* /*notification*/) = 0;
        /** Register for network notifications. */

        // PUBLIC_INTERFACE
        virtual uint32_t Unregister(INotification* /*notification*/) = 0;
        /** Unregister for network notifications. */

        // PUBLIC_INTERFACE
        virtual uint32_t GetAvailableInterfaces(IInterfaceDetailsIterator*& /*iterator @out */) = 0;
        /** Retrieve an iterator for available interfaces. */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetInternetConnectionStatus(bool& connected /* @out */) = 0;
        /** Retrieve basic internet connectivity status. */
    };

} // namespace Exchange
} // namespace WPEFramework
