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

#include "VideoClient.h"
#include "DeviceInfo.h"
#include "UtilsLogging.h"
#include "HttpClient.h"


namespace WPEFramework
{
    namespace Plugin
    {

        VideoClient::VideoClient(const string &host, const string &clientId, DeviceInfoPtr deviceInfoPtr)
            : mHost(host), mClientId(clientId), mDeviceInfoPtr(deviceInfoPtr)
        {
            ASSERT(deviceInfoPtr != nullptr);
        }

        uint32_t VideoClient::SignIn(const std::string& appId, const std::string& scope, bool isSignedIn)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string responseHeader;

            string partnerId;
            ret = mDeviceInfoPtr->GetPartnerId(partnerId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get partnerId: %d", ret);
                return ret;
            }

            string deviceId;
            ret = mDeviceInfoPtr->GetDeviceId(deviceId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get deviceId: %d", ret);
                return ret;
            }

            string accountId;
            ret = mDeviceInfoPtr->GetAccountId(accountId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get accountId: %d", ret);
                return ret;
            }

            string entityUrn = "xrn:xvp:application:" + appId;

            // {base}/partners/{partnerId}/accounts/{accountId}/videoServices/{entityUrn}/engaged
            // ?ownerReference=xrn:xcal:subscriber:{account|device}:{id}
            // &clientId=ripple
            string url = mHost + "/partners/" + partnerId + "/accounts/" + accountId + "/videoServices/" + entityUrn + "/engaged";

            if (scope == "device") {
                url += "?ownerReference=xrn:xcal:subscriber:device:" + deviceId;
            } else {
                url += "?ownerReference=xrn:xcal:subscriber:account:" + accountId;
            }

            url += "&clientId=" + mClientId;

            LOGINFO("url: %s", url.c_str());

            JsonObject bodyJson;
            bodyJson["eventType"] = "signIn";
            bodyJson["isSignedIn"] = isSignedIn;

            string body;
            bodyJson.ToString(body);

            LOGINFO("body: %s", body.c_str());

            ret = ProcessPut(url, body, response, responseHeader);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to process PUT request: %d", ret);
                return ret;
            }

            return ret;
        }

        uint32_t VideoClient::ProcessPut(const string &url, const string &data, string &response, string &responseHeader)
        {
            string token;
            uint32_t ret = mDeviceInfoPtr->GetServiceAccessToken(token);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get service access token: %d", ret);
                return ret;
            }

            long code = HttpClient::Put(url, token, data, response, responseHeader);
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
                code = HttpClient::Put(url, token, data, response, responseHeader);
            }

            if (code != 200)
            {
                LOGERR("Failed to process PUT: %ld resp: %s", code, response.c_str());
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