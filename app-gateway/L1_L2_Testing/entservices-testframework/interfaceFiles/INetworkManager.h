/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2023 RDK Management
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
**/

#pragma once
#include "Module.h"

// @stubgen:include <com/IIteratorType.h>
/* This referred from https://github.com/rdkcentral/networkmanager/blob/develop/interface/INetworkManager.h for Mock implementation*/

namespace WPEFramework
{
    namespace Exchange
    {
        enum myIDs {
            ID_NETWORKMANAGER                               = 0x800004E0,
            ID_NETWORKMANAGER_NOTIFICATION                  = ID_NETWORKMANAGER + 1
        };

        /* @json @text:keep */
        struct EXTERNAL INetworkManager: virtual public Core::IUnknown
        {
            // All interfaces require a unique ID, defined in Ids.h
            enum { ID = ID_NETWORKMANAGER };

            // Define the RPC methods
            enum InterfaceType : uint8_t {
                INTERFACE_TYPE_ETHERNET /* @text: ETHERNET */,
                INTERFACE_TYPE_WIFI     /* @text: WIFI */,
                INTERFACE_TYPE_P2P      /* @text: P2P */,
            };

            enum IPVersion : uint8_t
            {
                IP_ADDRESS_V4       /* @text: IPv4 */,
                IP_ADDRESS_V6       /* @text: IPv6 */,
            };

            enum IPStatus : uint8_t
            {
                IP_LOST     /* @text: LOST */,
                IP_ACQUIRED /* @text: ACQUIRED */,
            };

            struct EXTERNAL IPAddress {
                string ipversion    /* @text: ipversion */;
                bool autoconfig     /* @text: autoconfig */;
                string dhcpserver   /* @text: dhcpserver */;
                string ula          /* @text: ula */;
                string ipaddress    /* @text: ipaddress */;
                uint32_t prefix     /* @text: prefix */;
                string gateway      /* @text: gateway */;
                string primarydns   /* @text: primarydns */;
                string secondarydns /* @text: secondarydns */;
            };

            // Define the RPC methods
            enum InternetStatus : uint8_t
            {
                INTERNET_NOT_AVAILABLE      /* @text: NO_INTERNET */,
                INTERNET_LIMITED            /* @text: LIMITED_INTERNET */,
                INTERNET_CAPTIVE_PORTAL     /* @text: CAPTIVE_PORTAL */,
                INTERNET_FULLY_CONNECTED    /* @text: FULLY_CONNECTED */,
                INTERNET_UNKNOWN            /* @text: UNKNOWN */,
            };

            enum WiFiSignalQuality : uint8_t
            {
                WIFI_SIGNAL_DISCONNECTED    /* @text: Disconnected */,
                WIFI_SIGNAL_WEAK            /* @text: Weak */,
                WIFI_SIGNAL_FAIR            /* @text: Fair */,
                WIFI_SIGNAL_GOOD            /* @text: Good */,
                WIFI_SIGNAL_EXCELLENT       /* @text: Excellent */
            };

           // The state of the interface 
            enum InterfaceState : uint8_t
            {
                INTERFACE_ADDED,
                INTERFACE_LINK_UP,
                INTERFACE_LINK_DOWN,
                INTERFACE_ACQUIRING_IP,
                INTERFACE_REMOVED,
                INTERFACE_DISABLED
            };

            enum WiFiState : uint8_t
            {
                WIFI_STATE_UNINSTALLED,
                WIFI_STATE_DISABLED,
                WIFI_STATE_DISCONNECTED,
                WIFI_STATE_PAIRING,
                WIFI_STATE_CONNECTING,
                WIFI_STATE_CONNECTED,
                WIFI_STATE_SSID_NOT_FOUND,
                WIFI_STATE_SSID_CHANGED,
                WIFI_STATE_CONNECTION_LOST,
                WIFI_STATE_CONNECTION_FAILED,
                WIFI_STATE_CONNECTION_INTERRUPTED,
                WIFI_STATE_INVALID_CREDENTIALS,
                WIFI_STATE_AUTHENTICATION_FAILED,
                WIFI_STATE_ERROR,
                WIFI_STATE_INVALID
            };

            using IStringIterator           = RPC::IIteratorType<string,               RPC::ID_STRINGITERATOR>;

            /* @brief Get the Primary Interface used for external world communication */
            virtual uint32_t GetPrimaryInterface (string& interface /* @out */) = 0;

            /* @brief Enable/Disable the given interface */
            virtual uint32_t GetIPSettings(string& interface /* @inout */, const string& ipversion /* @in */, IPAddress& address /* @out */) = 0;

            /* @event */
            struct EXTERNAL INotification : virtual public Core::IUnknown
            {
                enum { ID = ID_NETWORKMANAGER_NOTIFICATION };

                // Network Notifications that other processes can subscribe to
                virtual void onInterfaceStateChange(const InterfaceState state /* @in */, const string interface /* @in */) = 0;
                virtual void onActiveInterfaceChange(const string prevActiveInterface /* @in */, const string currentActiveInterface /* @in */) = 0;
                virtual void onIPAddressChange(const string interface /* @in */, const string ipversion /* @in */, const string ipaddress /* @in */, const IPStatus status /* @in */) = 0;
                virtual void onInternetStatusChange(const InternetStatus prevState /* @in */, const InternetStatus currState /* @in */, const string interface /* @in */) = 0;

                // WiFi Notifications that other processes can subscribe to
                virtual void onAvailableSSIDs(const string jsonOfScanResults /* @in */) = 0;
                virtual void onWiFiStateChange(const WiFiState state /* @in */) = 0;
                virtual void onWiFiSignalQualityChange(const string ssid /* @in */, const string strength /* @in */, const string noise /* @in */, const string snr /* @in */, const WiFiSignalQuality quality /* @in */) = 0;
            };

            // Allow other processes to register/unregister from our notifications
            virtual uint32_t Register(INetworkManager::INotification* notification) = 0;
            virtual uint32_t Unregister(INetworkManager::INotification* notification) = 0;
        };
    }
}
