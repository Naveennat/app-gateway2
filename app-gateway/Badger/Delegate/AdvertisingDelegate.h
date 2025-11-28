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
#include "UtilsLogging.h"

#ifndef FB_ADVERTISING_CALLSIGN
#define FB_ADVERTISING_CALLSIGN "org.rdk.FbAdvertising"
#endif

#define ADVERTISING_APP_BUNDLE_ID_SUFFIX "Comcast"

namespace WPEFramework {
    class AdvertisingDelegate {
      public:
        AdvertisingDelegate(PluginHost::IShell* shell) : _shell(shell) {}

        ~AdvertisingDelegate() = default;

      public:
        uint32_t GetAppBundleId(const std::string& appId, std::string& appBundleId) {
            appBundleId = appId + "." + ADVERTISING_APP_BUNDLE_ID_SUFFIX;
            return Core::ERROR_NONE;
        }

        uint32_t LimitAdTracking(const std::string& appId, bool& limitAdTracking) {
            auto link = DelegateUtils::AcquireLink(_shell, FB_ADVERTISING_CALLSIGN);
            if (!link) {
                LOGWARN("Advertising link unavailable, returning empty object");
                limitAdTracking = false;
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;
            JsonObject context;
            context["appId"] = appId;
            params["context"] = context;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("policy"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("policy RPC failed, rc=%u", rc);
                limitAdTracking = false;
                return rc;
            }

            limitAdTracking = response["limitAdTracking"].Boolean();  // Add .Boolean() call
            return Core::ERROR_NONE;
        }

        uint32_t DeviceAdAttributes(const std::string& appId, std::string& adAttributesJson) {
            adAttributesJson.clear();

            auto link = DelegateUtils::AcquireLink(_shell, FB_ADVERTISING_CALLSIGN);
            if (!link) {
                LOGWARN("DeviceAdAttributes(): Advertising link unavailable, returning empty object");
                adAttributesJson = "{}";
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;
            JsonObject context;
            context["appId"] = appId;     // Create context object first
            params["context"] = context;  // Then assign it to params

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("deviceAttributes"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("DeviceAdAttributes(): deviceAttributes RPC failed, rc=%u", rc);
                adAttributesJson = "{}";
                return rc;
            }
            response.ToString(adAttributesJson);

            LOGINFO("DeviceAdAttributes JSON: %s", adAttributesJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t InitObject(const std::string& appId, const std::string& options, std::string result) {
            auto link = DelegateUtils::AcquireLink(_shell, FB_ADVERTISING_CALLSIGN);
            if (!link) {
                LOGWARN("Advertising link unavailable, returning empty object");
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject response;
            JsonObject contextParam;
            contextParam["appId"] = appId;

            JsonObject params;
            params["context"] = contextParam;
            params["options"] = options;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("config"), params, response);
            if (rc != Core::ERROR_NONE) {
                LOGERR("config RPC failed, rc=%u", rc);
                return rc;
            }

            return Core::ERROR_NONE;
        }

      private:
        PluginHost::IShell* _shell;
    };
}  // namespace WPEFramework