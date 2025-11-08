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
#ifndef NETWORK_CALLSIGN
#define NETWORK_CALLSIGN "org.rdk.Network"
#endif

namespace WPEFramework {
    class NetworkDelegate {
      public:
        NetworkDelegate(PluginHost::IShell* shell) : _shell(shell) {}

        ~NetworkDelegate() = default;

      public:
        uint32_t GetNetworkConnectivity(std::string& connectivityJson) {
            connectivityJson.clear();

            auto link = DelegateUtils::AcquireLink(_shell, NETWORK_CALLSIGN);
            if (!link) {
                connectivityJson = R"({"status":"NO_ACTIVE_NETWORK_INTERFACE","networkInterface":"UNKNOWN"})";
                LOGWARN("GetNetworkConnectivity(): link unavailable, returning fallback");
                return Core::ERROR_UNAVAILABLE;
            }

            JsonObject params;
            JsonObject response;

            uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getDefaultInterface"), params, response);
            if (rc != Core::ERROR_NONE || !response.HasLabel("interface")) {
                connectivityJson = R"({"status":"NO_ACTIVE_NETWORK_INTERFACE","networkInterface":"UNKNOWN"})";
                LOGERR("getDefaultInterface failed or missing interface, rc=%u", rc);
                return rc;
            }

            std::string iface = response["interface"].String();
            std::string mappedType = "UNKNOWN";

            if (iface.find("wlan") == 0 || iface.find("wifi") == 0) {
                mappedType = "WIFI";
            } else if (iface.find("eth") == 0) {
                mappedType = "ETHERNET";
            } else if (iface.find("moca") == 0) {
                mappedType = "MOCA";
            } else if (iface.find("docsis") == 0) {
                mappedType = "DOCSIS";
            }

            WPEFramework::Core::JSON::VariantContainer result;
            result["status"] = "SUCCESS";
            result["networkInterface"] = mappedType;

            result.ToString(connectivityJson);

            LOGINFO("NetworkConnectivity JSON: %s", connectivityJson.c_str());
            return Core::ERROR_NONE;
        }

      private:
        PluginHost::IShell* _shell;
    };
}  // namespace WPEFramework