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

// SystemDelegate: centralizes interactions with org.rdk.System for the Badger module.
#include "DelegateUtils.h"
#include <unordered_set>
#include <vector>

// Define a callsign constant to match the AUTHSERVICE_CALLSIGN-style pattern.
#ifndef SYSTEM_CALLSIGN
#define SYSTEM_CALLSIGN "org.rdk.System"
#endif

class SystemDelegate {
  public:
    // PUBLIC_INTERFACE
    /**
     * Create a SystemDelegate bound to the provided shell.
     */
    SystemDelegate(PluginHost::IShell* shell) : _shell(shell) {}

    ~SystemDelegate() = default;

  public:
    // PUBLIC_INTERFACE
    /**
     * GetFriendlyName
     * Calls org.rdk.System.getFriendlyName and returns a JSON string:
     *   {"friendly_name":"<Name>"}
     */
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
            LOGERR("getFriendlyName failed, rc=%u", rc);

            friendlyNameJson = R"({"friendly_name":"Living Room"})";
            return rc;
        }

        WPEFramework::Core::JSON::VariantContainer result;

        if (response.HasLabel("friendlyName")) {
            const string name = response["friendlyName"].String();
            if (!name.empty()) {
                result["friendly_name"] = name;
            } else {
                LOGERR("getFriendlyName returned empty 'friendlyName'");
                friendlyNameJson = R"({"friendly_name":"Living Room"})";
                return Core::ERROR_GENERAL;
            }
        } else {
            LOGERR("getFriendlyName response missing 'friendlyName'");
            friendlyNameJson = R"({"friendly_name":"Living Room"})";
            return Core::ERROR_GENERAL;
        }

        DelegateUtils::SerializeToJsonString(result, friendlyNameJson);

        LOGINFO("FriendlyName JSON: %s", friendlyNameJson.c_str());
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * GetTimeZone
     * Retrieve timezone using org.rdk.System.getTimeZoneDST and return JSON:
     *   {"time_zone":"<tz>"}
     */
    Core::hresult GetTimeZone(std::string& tzJson) {
        tzJson.clear();
        auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
        if (!link) {
            return Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;
        const uint32_t rc = link->Invoke<decltype(params), decltype(response)>("getTimeZoneDST", params, response);
        if (rc == Core::ERROR_NONE && response.HasLabel(_T("success")) && response[_T("success")].Boolean()) {
            if (response.HasLabel(_T("timeZone"))) {
                const string tz = response[_T("timeZone")].String();
                WPEFramework::Core::JSON::VariantContainer out;
                out["time_zone"] = tz.empty() ? string("UNKNOWN") : tz;
                DelegateUtils::SerializeToJsonString(out, tzJson);
                return Core::ERROR_NONE;
            }
        }
        LOGERR("SystemDelegate: couldn't get timezone");
        return Core::ERROR_UNAVAILABLE;
    }

    // PUBLIC_INTERFACE
    /**
     * GetSystemPlatformConfiguration
     * Calls org.rdk.System.getPlatformConfiguration and flattens the response into a JSON
     * with fields:
     *  account_id, device_id, model, device_type, supports_true_sd, browser_type, browser_version, browser_user_agent
     */
    uint32_t GetSystemPlatformConfiguration(std::string& platformConfigJson) {
        platformConfigJson.clear();

        auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
        if (!link) {
            // Fallback JSON
            platformConfigJson = R"({
            "account_id": "UNKNOWN",
            "device_id": "UNKNOWN",
            "model": "UNKNOWN",
            "device_type": "UNKNOWN",
            "supports_true_sd": false,
            "browser_type": "UNKNOWN",
            "browser_version": "UNKNOWN",
            "browser_user_agent": "UNKNOWN"
        })";
            LOGWARN("GetSystemPlatformConfiguration(): link unavailable, returning fallback");
            return Core::ERROR_UNAVAILABLE;
        }

        JsonObject params;
        JsonObject response;
        params["query"] = "";  // required by Thunder RPC

        uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getPlatformConfiguration"), params, response);
        if (rc != Core::ERROR_NONE) {
            LOGERR("getPlatformConfiguration failed, rc=%u", rc);

            platformConfigJson = R"({
            "account_id": "UNKNOWN",
            "device_id": "UNKNOWN",
            "model": "UNKNOWN",
            "device_type": "UNKNOWN",
            "supports_true_sd": false,
            "browser_type": "UNKNOWN",
            "browser_version": "UNKNOWN",
            "browser_user_agent": "UNKNOWN"
        })";
            return rc;
        }

        WPEFramework::Core::JSON::VariantContainer result;

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

        DelegateUtils::SerializeToJsonString(result, platformConfigJson);

        LOGINFO("PlatformConfig JSON: %s", platformConfigJson.c_str());
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * GetDeviceId
     * Utility that uses platform configuration to build:
     *   {"device_id":"<id>"}
     */
    uint32_t GetDeviceId(std::string& deviceIdJson) {
        deviceIdJson.clear();

        std::string platformConfigJson;
        uint32_t rc = GetSystemPlatformConfiguration(platformConfigJson);
        if (rc != Core::ERROR_NONE) {
            LOGWARN("GetDeviceId(): Platform configuration unavailable, returning fallback");
            deviceIdJson = R"({"device_id":"UNKNOWN"})";
            return Core::ERROR_NONE;
        }

        WPEFramework::Core::JSON::VariantContainer platformConfig;
        platformConfig.FromString(platformConfigJson);

        std::string id = platformConfig["device_id"].IsString() ? platformConfig["device_id"].String() : "UNKNOWN";

        if (id.empty()) {
            id = "UNKNOWN";
        }

        WPEFramework::Core::JSON::VariantContainer result;
        result["device_id"] = id;

        DelegateUtils::SerializeToJsonString(result, deviceIdJson);

        LOGINFO("DeviceId JSON: %s", deviceIdJson.c_str());
        return Core::ERROR_NONE;
    }

    // PUBLIC_INTERFACE
    /**
     * BuildSettings
     * Aggregates system-scoped settings for the requested keys. Supported keys:
     *  - friendly_name     -> calls getFriendlyName and wraps as {"friendly_name":{"value":"\"<Name>\""}}
     *  - legacyMiniGuide   -> {}
     *  - power_save_status -> {}
     */
    Core::hresult BuildSettings(const std::vector<std::string>& keys,
                                WPEFramework::Core::JSON::VariantContainer& out) const
    {
        using namespace WPEFramework;

        if (_shell == nullptr) {
            LOGERR("[badger.system] Shell not initialized");
            return Core::ERROR_UNAVAILABLE;
        }

        std::unordered_set<std::string> wanted;
        wanted.reserve(keys.size());
        for (const auto& k : keys) {
            wanted.insert(k);
        }

        // friendly_name -> org.rdk.System.getFriendlyName => {"friendly_name":{"value":"\"<Name>\""}}
        if (wanted.find("friendly_name") != wanted.end()) {
            auto link = DelegateUtils::AcquireLink(_shell, SYSTEM_CALLSIGN);
            if (link) {
                JsonObject params;
                JsonObject response;
                uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getFriendlyName"), params, response);
                if (rc == Core::ERROR_NONE) {
                    if (response.HasLabel("friendlyName")) {
                        std::string friendly = response["friendlyName"].String();
                        Core::JSON::VariantContainer obj;
                        // Wrap in quotes so JSON string contains escaped quotes, matching required shape
                        std::string wrapped = std::string("\"") + friendly + std::string("\"");
                        obj["value"] = wrapped;
                        out["friendly_name"] = obj;
                    } else {
                        LOGWARN("[badger.system] getFriendlyName missing 'friendlyName' field");
                    }
                } else {
                    LOGWARN("[badger.system] getFriendlyName failed rc=%u", rc);
                }
            } else {
                LOGWARN("[badger.system] No JSONRPC link for %s", SYSTEM_CALLSIGN);
            }
        }

        // legacyMiniGuide -> {}
        if (wanted.find("legacyMiniGuide") != wanted.end()) {
            Core::JSON::VariantContainer emptyObj;
            out["legacyMiniGuide"] = emptyObj;
        }

        // power_save_status -> {}
        if (wanted.find("power_save_status") != wanted.end()) {
            Core::JSON::VariantContainer emptyObj;
            out["power_save_status"] = emptyObj;
        }

        return Core::ERROR_NONE;
    }

  private:
    PluginHost::IShell* _shell;
    mutable Core::CriticalSection mAdminLock;
};
