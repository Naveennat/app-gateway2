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

#include "SessionClient.h"
#include "DeviceInfo.h"
#include "UtilsLogging.h"
#include "HttpClient.h"


namespace WPEFramework
{
    namespace Plugin
    {

        SessionClient::SessionClient(const string &host, const string &clientId, DeviceInfoPtr deviceInfoPtr)
            : mHost(host), mClientId(clientId), mDeviceInfoPtr(deviceInfoPtr)
        {
            ASSERT(deviceInfoPtr != nullptr);
        }

        uint32_t SessionClient::SetContentAccess(const string &appId, const string &availabilities, const string &entitlements)
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

            // {base}/partners/{partnerId}/accounts/{accountId}/appSettings/{appId}?deviceId={deviceId}&clientId={clientId}
            string url = mHost + "/partners/" + partnerId + "/accounts/" + accountId + "/appSettings/" + appId
                        + "?deviceId=" + deviceId + "&clientId=" + mClientId;


            LOGINFO("url: %s", url.c_str());

            JsonArray availabilitiesJson;
            JsonArray entitlementsJson;

            if (availabilities.empty() == false && availabilitiesJson.FromString(availabilities) == false)
            {
                LOGWARN("Failed to parse availabilities JSON: %s", availabilities.c_str());
            }
            if (entitlements.empty() == false && entitlementsJson.FromString(entitlements) == false)
            {
                LOGWARN("Failed to parse entitlements JSON: %s", entitlements.c_str());
            }

            JsonObject bodyJson;
            // Allow setting empty arrays but only if array is available
            // To be able to clear content access
            if (availabilities.empty() == false)
            {
                bodyJson["availabilities"] = availabilitiesJson;
            }

            if (entitlements.empty() == false)
            {
                bodyJson["entitlements"] = entitlementsJson;
            }

            string body;
            bodyJson.ToString(body);

            LOGINFO("body: %s", body.c_str());

            ret = ProcessPut(url, body, response, responseHeader);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to process PUT: %d", ret);
                return ret;
            }

            return ret;
        }

        uint32_t SessionClient::ClearContentAccess(const string &appId)
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

            // {base}/partners/{partnerId}/accounts/{accountId}/appSettings/{appId}?deviceId={deviceId}&clientId={clientId}
            string url = mHost + "/partners/" + partnerId + "/accounts/" + accountId + "/appSettings/" + appId
                         + "?deviceId=" + deviceId + "&clientId=" + mClientId;


            LOGINFO("url: %s", url.c_str());

            JsonObject bodyJson;
            JsonArray emptyArray;
            // Set empty arrays to clear content access
            bodyJson["availabilities"] = emptyArray;
            bodyJson["entitlements"] = emptyArray;

            string body;
            bodyJson.ToString(body);

            LOGINFO("body: %s", body.c_str());

            ret = ProcessPut(url, body, response, responseHeader);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to process PUT: %d", ret);
                return ret;
            }

            return ret;
        }

        uint32_t SessionClient::ProcessPut(const string &url, const string &data, string &response, string &responseHeader)
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

            if (code != 200 && code != 204)
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