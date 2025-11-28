/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "XifaClient.h"
#include "DeviceInfo.h"
#include "UtilsLogging.h"
#include "HttpClient.h"
#include <interfaces/IPrivacy.h>

#define PRIVACY_PLUGIN_CALLSIGN "org.rdk.Privacy"

namespace WPEFramework
{
    namespace Plugin
    {

        XifaClient::XifaClient(PluginHost::IShell* shell, const string &host, const string &clientId, DeviceInfoPtr deviceInfoPtr)
            : mHost(host), mClientId(clientId), mDeviceInfoPtr(deviceInfoPtr), mShell(shell)
        {
            ASSERT(deviceInfoPtr != nullptr);
            ASSERT(shell != nullptr);
            mShell->AddRef();
        }

        XifaClient::~XifaClient()
        {
            LOGINFO("~XifaClient destructor");
            if (mShell)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        class XifaAdIdentifierResp: public Core::JSON::Container
        {
        private:
            XifaAdIdentifierResp(const XifaAdIdentifierResp &) = delete;
            XifaAdIdentifierResp &operator=(const XifaAdIdentifierResp &) = delete;

        public:
            XifaAdIdentifierResp()
                : Core::JSON::Container(), Xifa(), XifaType()
            {
                Add(_T("xifa"), &Xifa);
                Add(_T("xifaType"), &XifaType);
            }

            Core::JSON::String Xifa;
            Core::JSON::String XifaType;
        };

        uint32_t XifaClient::GetAdIdentifier(const string &appId, const string &scopeType, const std::string &scopeId,
                                              string &ifa, string &ifa_type, string &lmt)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string responseHeader;

            if (appId.empty())
            {
                LOGERR("appId is empty");
                return Core::ERROR_BAD_REQUEST;
            }

            string partnerId;
            ret = mDeviceInfoPtr->GetPartnerId(partnerId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get partnerId: %d", ret);
                return ret;
            }

            string accountId;
            ret = mDeviceInfoPtr->GetAccountId(accountId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get accountId: %d", ret);
                return ret;
            }

            string deviceId;
            ret = mDeviceInfoPtr->GetDeviceId(deviceId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get deviceId: %d", ret);
                return ret;
            }

            bool privacyAdTracking = false;

            auto privacy = mShell->QueryInterfaceByCallsign<Exchange::IPrivacy>(PRIVACY_PLUGIN_CALLSIGN);
            if (privacy != nullptr)
            {
                Exchange::IPrivacy::PrivacySettingOutData data;
                uint32_t err = privacy->GetAppContentAdTargeting(data);
                if (err == Core::ERROR_NONE)
                {
                    privacyAdTracking = data.allowed;
                }
                else
                {
                    LOGERR("Failed to get ad tracking privacy setting: %d", err);
                }
                privacy->Release();
            }

            string url = mHost + "/partners/" + partnerId + "/accounts/" + accountId
                        + "/devices/" + deviceId + "/autoresolve/xifa?adTrackingParticipationState=" + (privacyAdTracking ? "true" : "false")
                        + "&clientId=" + mClientId;

            url += "&entityScopeId=xrn:xvp:application:" + appId;
            if (scopeId.empty() == false && scopeType.empty() == false)
            {
                url += ":" + scopeType + ":" + scopeId;
            }

            LOGINFO("url: %s", url.c_str());

            ret = ProcessGet(url, response, responseHeader);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to process GET: %d", ret);
                return ret;
            }

            LOGINFO("Response: %s", response.c_str());

            XifaAdIdentifierResp responseJson;
            if (responseJson.FromString(response) == false)
            {
                LOGERR("Failed to parse response json");
                return Core::ERROR_GENERAL;
            }

            ifa = responseJson.Xifa.Value();
            ifa_type = responseJson.XifaType.Value();
            lmt = privacyAdTracking ? "0" : "1";

            return ret;
        }

        uint32_t XifaClient::ResetAdIdentifier(const string &appId, const string &scopeType, const std::string &scopeId)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string responseHeader;

            if (appId.empty())
            {
                LOGERR("appId is empty");
                return Core::ERROR_BAD_REQUEST;
            }

            string partnerId;
            ret = mDeviceInfoPtr->GetPartnerId(partnerId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get partnerId: %d", ret);
                return ret;
            }

            string accountId;
            ret = mDeviceInfoPtr->GetAccountId(accountId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get accountId: %d", ret);
                return ret;
            }

            string url = mHost + "/partners/" + partnerId + "/accounts/" + accountId + "/xifas/reset"
                        + "?clientId=" + mClientId;

            url += "&entityScopeId=xrn:xvp:application:" + appId;
            if (scopeId.empty() == false && scopeType.empty() == false)
            {
                url += ":" + scopeType + ":" + scopeId;
            }

            LOGINFO("url: %s", url.c_str());

            ret = ProcessPost(url, "", response, responseHeader);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to Process POST: %d", ret);
                return ret;
            }

            LOGINFO("Response: %s", response.c_str());

            return ret;
        }

        uint32_t XifaClient::ProcessGet(const string &url, std::string &response, std::string &responseHeader)
        {
            string token;
            uint32_t ret = mDeviceInfoPtr->GetServiceAccessToken(token);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get service access token: %d", ret);
                return ret;
            }

            long code = HttpClient::Get(url, token, response, responseHeader);
            if (code == 401)
            {
                // token expired, get a new one
                ret = mDeviceInfoPtr->GetServiceAccessToken(token);
                if (ret != Core::ERROR_NONE)
                {
                    LOGERR("Failed to get service access token: %d", ret);
                    return ret;
                }
                // retry
                code = HttpClient::Get(url, token, response, responseHeader);
            }

            if (code != 200)
            {
                LOGERR("Failed to process GET: %ld resp: %s", code, response.c_str());
                switch (code)
                {
                case 404:
                    return Core::ERROR_UNAVAILABLE;
                case 400:
                case 403:
                    return Core::ERROR_NOT_SUPPORTED;
                case 500:
                case 502:
                case 503:
                case 0:
                    return Core::ERROR_UNREACHABLE_NETWORK;
                }
                return Core::ERROR_GENERAL;
            }
            return ret;
        }

        uint32_t XifaClient::ProcessPost(const string &url, const string &data, string &response, string &responseHeader)
        {
            string token;
            uint32_t ret = mDeviceInfoPtr->GetServiceAccessToken(token);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get service access token: %d", ret);
                return ret;
            }

            long code = HttpClient::Post(url, token, data, response, responseHeader);
            if (code == 401)
            {
                // token expired, get a new one
                ret = mDeviceInfoPtr->GetServiceAccessToken(token);
                if (ret != Core::ERROR_NONE)
                {
                    LOGERR("Failed to get service access token: %d", ret);
                    return ret;
                }
                // retry
                code = HttpClient::Post(url, token, data, response, responseHeader);
            }

            if (code != 200)
            {
                LOGERR("Failed to process POST: %ld resp: %s", code, response.c_str());
                switch (code)
                {
                case 404:
                    return Core::ERROR_UNAVAILABLE;
                case 400:
                    return Core::ERROR_NOT_SUPPORTED;
                case 0:
                    return Core::ERROR_UNREACHABLE_NETWORK;
                }

                return Core::ERROR_GENERAL;
            }
            return ret;
        }

    }
}