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
#ifndef AUTHSERVICE_CALLSIGN
#define AUTHSERVICE_CALLSIGN "org.rdk.AuthService"
#endif

namespace WPEFramework {
    class AuthServiceDelegate {
      public:
        AuthServiceDelegate(PluginHost::IShell* shell) : _shell(shell) {}

        ~AuthServiceDelegate() = default;

      public:
      uint32_t GetXDeviceId(std::string& deviceId) {
            deviceId.clear();

            auto link = DelegateUtils::AcquireLink(_shell, AUTHSERVICE_CALLSIGN);
            if (!link) {
                LOGERR("GetXDeviceId(): link unavailable");
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getXDeviceId"), params, response);
            if (rc != Core::ERROR_NONE || !response.HasLabel("xDeviceId")) {
                LOGERR("getXDeviceId failed or missing xDeviceId, rc=%u", rc);
                return rc;
            }

            deviceId = response["xDeviceId"].String();
            LOGINFO("GetXDeviceId: %s", deviceId.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t GetAccountId(std::string& accountId) {
            accountId.clear();

            auto link = DelegateUtils::AcquireLink(_shell, AUTHSERVICE_CALLSIGN);
            if (!link) {
                LOGERR("GetAccountId(): link unavailable");
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getServiceAccountId"), params, response);
            if (rc != Core::ERROR_NONE || !response.HasLabel("serviceAccountId")) {
                LOGERR("getServiceAccountId failed or missing serviceAccountId, rc=%u", rc);
                return rc;
            }

            accountId = response["serviceAccountId"].String();
            LOGINFO("GetAccountId: %s", accountId.c_str());
            return Core::ERROR_NONE;
        }

      private:
        PluginHost::IShell* _shell;
    };
}  // namespace WPEFramework