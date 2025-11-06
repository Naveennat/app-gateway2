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

#include "PlaybackClient.h"
#include "DeviceInfo.h"
#include "UtilsLogging.h"
#include "HttpClient.h"

namespace WPEFramework
{
    namespace Plugin
    {

        PlaybackClient::PlaybackClient(PluginHost::IShell *shell, const string &host, const string &clientId, DeviceInfoPtr deviceInfoPtr)
            : mHost(host), mClientId(clientId), mDeviceInfoPtr(deviceInfoPtr), mDataGovernance(shell)
        {
            ASSERT(deviceInfoPtr != nullptr);
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();
        }

        PlaybackClient::~PlaybackClient()
        {
            LOGINFO("~PlaybackClient destructor");
            if (mShell)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        uint32_t PlaybackClient::PutResumePoint(const string &appId,
                                                const string &contentPartnerId,
                                                const string &contentId,
                                                const double progress,
                                                const bool completed,
                                                const string &watchedOn)
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

            // {base}/partners/{partnerId}/accounts/{accountId}/deviceId/{deviceId}/resumePoints/ott/{contentPartnerId}/{contentId}
            // ?clientId=ripple
            string url = mHost + "/partners/" + partnerId + "/accounts/" + accountId + "/deviceId/"
             + deviceId + "/resumePoints/ott/" + contentPartnerId + "/" + contentId + "?clientId=" + mClientId;

            LOGINFO("url: %s", url.c_str());

            JsonObject bodyJson;

            bodyJson["durableAppId"] = appId;
            if (progress >= 0.0 && progress <= 1.0)
            {
                bodyJson["progress"] = static_cast<uint32_t>(progress * 100);
                bodyJson["progressUnits"] = "Percent";
            }
            else
            {
                bodyJson["progress"] = static_cast<uint32_t>(progress);
                bodyJson["progressUnits"] = "Seconds";
            }

            bodyJson["completed"] = completed;

            // Get Tags
            DataGovernance::Tags tags;
            bool drop = false;
            ret = mDataGovernance.ResolveTags(appId, tags, drop);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to resolve tags: %d", ret);
                return ret;
            }

            // Compose JsonArray from tags[].name
            JsonArray cetJson;
            for (const auto &tag : tags)
            {
                cetJson.Add(tag.name);
            }
            bodyJson["cet"] = cetJson;

            // Compose JsonArray from tags[].name only for tags[].propagationState == false
            JsonArray cetNotPropagatedJson;
            for (const auto &tag : tags)
            {
                if (!tag.propagationState)
                {
                    cetNotPropagatedJson.Add(tag.name);
                }
            }
            bodyJson["cet_NotPropagated"] = cetNotPropagatedJson;

            bodyJson["ownerReference"] = "xrn:subscriber:device:" + deviceId;

            string body;
            bodyJson.ToString(body);

            LOGINFO("Body: %s", body.c_str());

            ret = ProcessPut(url, body, response, responseHeader);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to process PUT: %d", ret);
                return ret;
            }

            LOGINFO("Response: %s", response.c_str());

            JsonObject respJson;
            if (respJson.FromString(response) == false)
            {
                LOGERR("Failed to parse response: %s", response.c_str());
                return Core::ERROR_GENERAL;
            }

            // Expect response like:
            // {
            //   "messageId": "mid-123",
            //   "snsStatusCode": 200,
            //   "snsStatusText": "OK",
            //   "awsRequestId": "req-abc-123"
            // }
            if (respJson.HasLabel("snsStatusCode") && respJson["snsStatusCode"].Content() == Core::JSON::Variant::type::NUMBER && respJson["snsStatusCode"].Number() == 200)
            {
                return Core::ERROR_NONE;
            }

            return Core::ERROR_GENERAL;
        }

        uint32_t PlaybackClient::ProcessPut(const string &url, const string &data, string &response, string &responseHeader)
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