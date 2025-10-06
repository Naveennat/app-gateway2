#pragma once

/*
 * SystemDelegate encapsulates all org.rdk.System JSON-RPC calls and event subscriptions
 * required by the FbSettings plugin to fulfill the system settings alias functions.
 *
 * It uses a direct JSON-RPC link to the Thunder plugin via Utils::GetThunderControllerClient,
 * following the same pattern as the AUTHSERVICE_CALLSIGN-style setup (using helpers from Supporting_Files).
 */

#include <memory>
#include <string>
#include <unordered_set>

#include <plugins/plugins.h>
#include "UtilsLogging.h"
#include "UtilsJsonrpcDirectLink.h"

// Define a callsign constant to match the AUTHSERVICE_CALLSIGN-style pattern.
#ifndef SYSTEM_CALLSIGN
#define SYSTEM_CALLSIGN "org.rdk.System"
#endif

namespace WPEFramework {
namespace Plugin {

    class SystemDelegate {
    public:
        SystemDelegate()
            : _shell(nullptr)
        {
        }

        ~SystemDelegate() = default;

        // PUBLIC_INTERFACE
        void setShell(PluginHost::IShell* shell) {
            /**
             * Set the Thunder IShell pointer used to acquire a direct JSON-RPC link
             * to the org.rdk.System plugin.
             *
             * @param shell Pointer to the PluginHost::IShell provided by the framework.
             */
            _shell = shell;
        }

        // PUBLIC_INTERFACE
        bool HandleEvent(const std::string& event, const bool listen, bool& registrationError) {
            /**
             * Handle event registration requests; returns true if this delegate recognized the event.
             * This follows the same event naming as the System plugin.
             *
             * @param event Event name (case-insensitive), e.g. "Localization.onCountryCodeChanged"
             * @param listen True to subscribe, false to unsubscribe
             * @param registrationError Set to true on registration error
             * @return true if the event was handled by this delegate
             */
            registrationError = false;

            const std::string evLower = ToLower(event);
            if (evLower == "localization.oncountrycodechanged") {
                bool dummy = false;
                auto rc = SubscribeOnCountryCodeChanged(listen, dummy);
                registrationError = (rc != Core::ERROR_NONE);
                return true;
            } else if (evLower == "localization.ontimezonechanged") {
                bool dummy = false;
                auto rc = SubscribeOnTimeZoneChanged(listen, dummy);
                registrationError = (rc != Core::ERROR_NONE);
                return true;
            } else if (evLower == "secondscreen.onfriendlynamechanged") {
                bool dummy = false;
                auto rc = SubscribeOnFriendlyNameChanged(listen, dummy);
                registrationError = (rc != Core::ERROR_NONE);
                return true;
            } else if (evLower == "device.onnamechanged") {
                bool dummy = false;
                auto rc = SubscribeOnFriendlyNameChanged(listen, dummy);
                registrationError = (rc != Core::ERROR_NONE);
                return true;
            }

            return false;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetDeviceMake(std::string& make) {
            /** Retrieve the device make using org.rdk.System.getDeviceInfo */
            make.clear();
            auto link = AcquireLink();
            if (!link) {
                make = "unknown";
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getDeviceInfo", params, response);
            if (rc == Core::ERROR_NONE) {
                if (response.HasLabel(_T("make"))) {
                    make = response[_T("make")].String();
                }
            }

            if (make.empty()) {
                // Per transform: return_or_else(.result.make, "unknown")
                make = "unknown";
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetDeviceName(std::string& name) {
            /** Retrieve the friendly name using org.rdk.System.getFriendlyName */
            name.clear();
            auto link = AcquireLink();
            if (!link) {
                name = "Living Room";
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getFriendlyName", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel(_T("friendlyName"))) {
                name = response[_T("friendlyName")].String();
            }

            // Default if empty
            if (name.empty()) {
                name = "Living Room";
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SetDeviceName(const std::string& name) {
            /** Set the friendly name using org.rdk.System.setFriendlyName */
            auto link = AcquireLink();
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            params[_T("friendlyName")] = name;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("setFriendlyName", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean()) {
                return Core::ERROR_NONE;
            }
            LOGERR("SystemDelegate: couldn't set name");
            return Core::ERROR_GENERAL;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetDeviceSku(std::string& skuOut) {
            /** Retrieve the device SKU from org.rdk.System.getSystemVersions.stbVersion */
            skuOut.clear();
            auto link = AcquireLink();
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getSystemVersions", params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("SystemDelegate: getSystemVersions failed rc=%u", rc);
                return Core::ERROR_UNAVAILABLE;
            }
            if (!response.HasLabel(_T("stbVersion"))) {
                LOGERR("SystemDelegate: getSystemVersions missing stbVersion");
                return Core::ERROR_UNAVAILABLE;
            }

            const std::string stbVersion = response[_T("stbVersion")].String();
            // Per transform: split("_")[0]
            auto pos = stbVersion.find('_');
            skuOut = (pos == std::string::npos) ? stbVersion : stbVersion.substr(0, pos);
            if (skuOut.empty()) {
                LOGERR("SystemDelegate: Failed to get SKU");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetCountryCode(std::string& code) {
            /** Retrieve Firebolt country code derived from org.rdk.System.getTerritory */
            code.clear();
            auto link = AcquireLink();
            if (!link) {
                code = "US";
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getTerritory", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel(_T("territory"))) {
                const std::string terr = response[_T("territory")].String();
                code = TerritoryThunderToFirebolt(terr, "US");
            }
            if (code.empty()) {
                code = "US";
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SetCountryCode(const std::string& code) {
            /** Set territory using org.rdk.System.setTerritory mapped from Firebolt country code */
            auto link = AcquireLink();
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            const std::string territory = TerritoryFireboltToThunder(code, "USA");
            WPEFramework::Core::JSON::VariantContainer params;
            params[_T("territory")] = territory;

            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("setTerritory", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean()) {
                return Core::ERROR_NONE;
            }
            LOGERR("SystemDelegate: couldn't set countrycode");
            return Core::ERROR_GENERAL;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnCountryCodeChanged(const bool listen, bool& status) {
            /**
             * Stubbed subscription to org.rdk.System.onTerritoryChanged.
             * Records intent and returns success in this environment.
             */
            status = true;
            if (listen) {
                _subscriptions.insert("onTerritoryChanged");
                LOGINFO("SystemDelegate: requested subscribe to org.rdk.System.onTerritoryChanged (not implemented)");
            } else {
                _subscriptions.erase("onTerritoryChanged");
                LOGINFO("SystemDelegate: requested unsubscribe from org.rdk.System.onTerritoryChanged (not implemented)");
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetTimeZone(std::string& tz) {
            /** Retrieve timezone using org.rdk.System.getTimeZoneDST */
            tz.clear();
            auto link = AcquireLink();
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getTimeZoneDST", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean()) {
                if (response.HasLabel(_T("timeZone"))) {
                    tz = response[_T("timeZone")].String();
                    return Core::ERROR_NONE;
                }
            }
            LOGERR("SystemDelegate: couldn't get timezone");
            return Core::ERROR_UNAVAILABLE;
        }

        // PUBLIC_INTERFACE
        Core::hresult SetTimeZone(const std::string& tz) {
            /** Set timezone using org.rdk.System.setTimeZoneDST */
            auto link = AcquireLink();
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            WPEFramework::Core::JSON::VariantContainer params;
            params[_T("timeZone")] = tz;
            WPEFramework::Core::JSON::VariantContainer response;
            const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("setTimeZoneDST", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean()) {
                return Core::ERROR_NONE;
            }
            LOGERR("SystemDelegate: couldn't set timezone");
            return Core::ERROR_GENERAL;
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnTimeZoneChanged(const bool listen, bool& status) {
            /** Stubbed subscription to org.rdk.System.onTimeZoneDSTChanged */
            status = true;
            if (listen) {
                _subscriptions.insert("onTimeZoneDSTChanged");
                LOGINFO("SystemDelegate: requested subscribe to org.rdk.System.onTimeZoneDSTChanged (not implemented)");
            } else {
                _subscriptions.erase("onTimeZoneDSTChanged");
                LOGINFO("SystemDelegate: requested unsubscribe from org.rdk.System.onTimeZoneDSTChanged (not implemented)");
            }
            return Core::ERROR_NONE;
        }

        // PUBLIC_INTERFACE
        Core::hresult GetSecondScreenFriendlyName(std::string& name) {
            /** Alias to GetDeviceName using org.rdk.System.getFriendlyName */
            return GetDeviceName(name);
        }

        // PUBLIC_INTERFACE
        Core::hresult SubscribeOnFriendlyNameChanged(const bool listen, bool& status) {
            /** Stubbed subscription to org.rdk.System.onFriendlyNameChanged */
            status = true;
            if (listen) {
                _subscriptions.insert("onFriendlyNameChanged");
                LOGINFO("SystemDelegate: requested subscribe to org.rdk.System.onFriendlyNameChanged (not implemented)");
            } else {
                _subscriptions.erase("onFriendlyNameChanged");
                LOGINFO("SystemDelegate: requested unsubscribe from org.rdk.System.onFriendlyNameChanged (not implemented)");
            }
            return Core::ERROR_NONE;
        }

    private:
        inline std::unique_ptr<WPEFramework::Utils::JSONRPCDirectLink> AcquireLink() const {
            // Create a direct JSON-RPC link to the Thunder System plugin using the Supporting_Files helper.
            if (_shell == nullptr) {
                LOGERR("SystemDelegate: shell is null");
                return nullptr;
            }
            return WPEFramework::Utils::GetThunderControllerClient(_shell, SYSTEM_CALLSIGN);
        }

        static std::string ToLower(const std::string& in) {
            std::string out;
            out.reserve(in.size());
            for (char c : in) {
                out.push_back(static_cast<char>(::tolower(static_cast<unsigned char>(c))));
            }
            return out;
        }

        static std::string TerritoryThunderToFirebolt(const std::string& terr, const std::string& deflt) {
            if (EqualsIgnoreCase(terr, "USA")) return "US";
            if (EqualsIgnoreCase(terr, "CAN")) return "CA";
            return deflt;
        }

        static std::string TerritoryFireboltToThunder(const std::string& code, const std::string& deflt) {
            if (EqualsIgnoreCase(code, "US")) return "USA";
            if (EqualsIgnoreCase(code, "CA")) return "CAN";
            return deflt;
        }

        static bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i) {
                if (::tolower(static_cast<unsigned char>(a[i])) != ::tolower(static_cast<unsigned char>(b[i]))) {
                    return false;
                }
            }
            return true;
        }

    private:
        PluginHost::IShell* _shell;
        std::unordered_set<std::string> _subscriptions;
    };

} // namespace Plugin
} // namespace WPEFramework
