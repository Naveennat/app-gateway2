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
#ifndef HDCP_PROFILE_CALLSIGN
#define HDCP_PROFILE_CALLSIGN "org.rdk.HdcpProfile"
#endif

class HdcpProfileDelegate {
  public:
    HdcpProfileDelegate(PluginHost::IShell* shell) : _shell(shell) {}

    ~HdcpProfileDelegate() = default;

  public:
    uint32_t GetHDCPStatus(std::string& hdcpJson) {
        hdcpJson.clear();

        auto link = DelegateUtils::AcquireLink();
        if (!link) {
            hdcpJson = R"({
            "connected": false,
            "hdcp_compliant": false,
            "hdcp_enabled": false,
            "supported_hdcp_version": "UNKNOWN",
            "receiver_hdcp_version": "UNKNOWN",
            "current_hdcp_version": "UNKNOWN"
        })";
            LOGWARN("GetHDCPStatus(): link unavailable, returning default HDCP values");
            return Core::ERROR_UNAVAILABLE;
        }

        JsonObject params;
        JsonObject response;

        uint32_t rc = link->Invoke<JsonObject, JsonObject>(_T("getHDCPStatus"), params, response);
        if (rc != Core::ERROR_NONE || !response.HasLabel("HDCPStatus")) {
            LOGWARN("getHDCPStatus failed or missing HDCPStatus, rc=%u", rc);

            hdcpJson = R"({
            "connected": false,
            "hdcp_compliant": false,
            "hdcp_enabled": false,
            "supported_hdcp_version": "UNKNOWN",
            "receiver_hdcp_version": "UNKNOWN",
            "current_hdcp_version": "UNKNOWN"
        })";
            return rc;
        }

        const string hdcpRaw = response["HDCPStatus"].String();
        WPEFramework::Core::JSON::VariantContainer source;
        source.FromString(hdcpRaw);

        WPEFramework::Core::JSON::VariantContainer result;

        result["connected"] = source["connected"];
        result["hdcp_compliant"] = source["hdcp_compliant"];
        result["hdcp_enabled"] = source["hdcp_enabled"];
        result["supported_hdcp_version"] = source["supported_hdcp_version"];
        result["receiver_hdcp_version"] = source["receiver_hdcp_version"];
        result["current_hdcp_version"] = source["current_hdcp_version"];

        DelegateUtils::SerializeToJsonString(result, hdcpJson);

        LOGINFO("HDCP JSON: %s", hdcpJson.c_str());
        return Core::ERROR_NONE;
    }

  private:
    PluginHost::IShell* _shell;
    mutable Core::CriticalSection mAdminLock;
};
