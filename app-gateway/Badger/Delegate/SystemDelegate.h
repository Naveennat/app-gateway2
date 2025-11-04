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

class SystemDelegate {
  public:
    SystemDelegate(PluginHost::IShell* shell) : _shell(shell) {}

    ~SystemDelegate() = default;

  public:
    uint32_t GetFriendlyName(std::string& friendlyNameJson) {
        friendlyNameJson.clear();

        auto link = DelegateUtils::AcquireLink();
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
                // Wrap in quotes to make it a valid JSON string
                tz = "\"" + tz + "\"";
                return Core::ERROR_NONE;
            }
        }
        LOGERR("SystemDelegate: couldn't get timezone");
        return Core::ERROR_UNAVAILABLE;
    }

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

  private:
    PluginHost::IShell* _shell;
    mutable Core::CriticalSection mAdminLock;
};
