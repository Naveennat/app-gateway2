/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
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

#include "DelegateUtils.h"

// Define a callsign constant to match the AUTHSERVICE_CALLSIGN-style pattern.
#ifndef SYSTEM_CALLSIGN
#define SYSTEM_CALLSIGN "org.rdk.System"
#endif

namespace WPEFramework {

    class SystemDelegate {
      public:
        SystemDelegate(PluginHost::IShell* shell) : _shell(shell) {}
        ~SystemDelegate() = default;

      public:
        uint32_t GetFriendlyName(std::string& friendlyNameJson) {
            friendlyNameJson.clear();

            auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
            if (!link) {
                friendlyNameJson = R"({"friendly_name":"Living Room"})";
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getFriendlyName"), params, response);
            if (rc != Core::ERROR_NONE) {
                friendlyNameJson = R"({"friendly_name":"Living Room"})";
                return rc;
            }

            Core::JSON::VariantContainer result;

            if (response.HasLabel("friendlyName")) {
                std::string name = response["friendlyName"].String();
                result["friendly_name"] = name.empty() ? "Living Room" : name;
            } else {
                friendlyNameJson = R"({"friendly_name":"Living Room"})";
                return Core::ERROR_GENERAL;
            }

            result.ToString(friendlyNameJson);
            return Core::ERROR_NONE;
        }

        Core::hresult GetTimeZone(std::string& tzJson) {
            tzJson.clear();
            auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
            if (!link) {
                return Core::ERROR_UNAVAILABLE;
            }

            Core::JSON::VariantContainer params;
            Core::JSON::VariantContainer response;

            uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getTimeZoneDST", params, response);
            if (rc == Core::ERROR_NONE && response.HasLabel("success") && response["success"].Boolean()) {
                if (response.HasLabel("timeZone")) {
                    Core::JSON::VariantContainer res;
                    res["time_zone"] = response["timeZone"].String();
                    res.ToString(tzJson);
                    return Core::ERROR_NONE;
                }
            }

            return Core::ERROR_UNAVAILABLE;
        }

        uint32_t GetSystemPlatformConfiguration(std::string& platformConfigJson) {
            platformConfigJson.clear();

            auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
            if (!link) {
                platformConfigJson = R"({"account_id":"UNKNOWN","device_id":"UNKNOWN","model":"UNKNOWN","device_type":"UNKNOWN","supports_true_sd":false,"browser_type":"UNKNOWN","browser_version":"UNKNOWN","browser_user_agent":"UNKNOWN"})";
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;
            params["query"] = "";

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getPlatformConfiguration"), params, response);
            if (rc != Core::ERROR_NONE) {
                platformConfigJson = R"({"account_id":"UNKNOWN","device_id":"UNKNOWN","model":"UNKNOWN","device_type":"UNKNOWN","supports_true_sd":false,"browser_type":"UNKNOWN","browser_version":"UNKNOWN","browser_user_agent":"UNKNOWN"})";
                return rc;
            }

            Core::JSON::VariantContainer result;

            if (response.HasLabel("AccountInfo")) {
                auto acc = response["AccountInfo"].Object();
                result["account_id"] = acc.HasLabel("accountId") ? acc["accountId"].String() : "UNKNOWN";
                result["device_id"] = acc.HasLabel("x1DeviceId") ? acc["x1DeviceId"].String() : "UNKNOWN";
            }

            if (response.HasLabel("DeviceInfo")) {
                auto dev = response["DeviceInfo"].Object();
                result["model"] = dev.HasLabel("model") ? dev["model"].String() : "UNKNOWN";
                result["device_type"] = dev.HasLabel("deviceType") ? dev["deviceType"].String() : "UNKNOWN";
                result["supports_true_sd"] = dev.HasLabel("supportsTrueSD") ? dev["supportsTrueSD"].Boolean() : false;

                if (dev.HasLabel("webBrowser")) {
                    auto wb = dev["webBrowser"].Object();
                    result["browser_type"] = wb.HasLabel("browserType") ? wb["browserType"].String() : "UNKNOWN";
                    result["browser_version"] = wb.HasLabel("version") ? wb["version"].String() : "UNKNOWN";
                    result["browser_user_agent"] = wb.HasLabel("userAgent") ? wb["userAgent"].String() : "UNKNOWN";
                }
            }

            result.ToString(platformConfigJson);
            return Core::ERROR_NONE;
        }

        uint32_t GetDeviceId(std::string& deviceIdJson) {
            deviceIdJson.clear();

            std::string platformConfigJson;
            uint32_t rc = GetSystemPlatformConfiguration(platformConfigJson);

            Core::JSON::VariantContainer result;
            Core::JSON::VariantContainer platformConfig;
            platformConfig.FromString(platformConfigJson);

            std::string id = DelegateUtils::GetStringSafe(platformConfig, "device_id", "UNKNOWN");
            result["device_id"] = id;
            result.ToString(deviceIdJson);
            return Core::ERROR_NONE;
        }

        uint32_t GetCountryCode(std::string& countryCodeJson) {
            countryCodeJson.clear();

            auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
            if (!link) {
                countryCodeJson = R"({"country_code":"US"})";
                return Core::ERROR_UNAVAILABLE;
            }

            Core::JSON::VariantContainer params;
            Core::JSON::VariantContainer response;

            uint32_t rc = link->Invoke<decltype(params), decltype(response)>(_T("getTerritory"), params, response);
            std::string country = "US";

            if (rc == Core::ERROR_NONE && response.HasLabel("territory")) {
                std::string terr = response["territory"].String();
                if (!terr.empty()) {
                    country = TerritoryThunderToFirebolt(terr, "US");
                }
            }

            Core::JSON::VariantContainer result;
            result["country_code"] = country;
            result.ToString(countryCodeJson);
            return Core::ERROR_NONE;
        }

      private:
        static bool EqualsIgnoreCase(const std::string& a, const std::string& b) {
            if (a.size() != b.size()) return false;
            for (size_t i = 0; i < a.size(); ++i)
                if (tolower(a[i]) != tolower(b[i])) return false;
            return true;
        }

        static std::string TerritoryThunderToFirebolt(const std::string& terr, const std::string& deflt) {
            if (EqualsIgnoreCase(terr, "USA")) return "US";
            if (EqualsIgnoreCase(terr, "CAN")) return "CA";
            if (EqualsIgnoreCase(terr, "ITA")) return "IT";
            if (EqualsIgnoreCase(terr, "GBR")) return "GB";
            if (EqualsIgnoreCase(terr, "IRL")) return "IE";
            if (EqualsIgnoreCase(terr, "AUS")) return "AU";
            if (EqualsIgnoreCase(terr, "AUT")) return "AT";
            if (EqualsIgnoreCase(terr, "CHE")) return "CH";
            if (EqualsIgnoreCase(terr, "DEU")) return "DE";
            return deflt;
        }

      private:
        PluginHost::IShell* _shell;
    };

} // namespace WPEFramework
