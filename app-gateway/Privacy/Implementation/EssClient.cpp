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

#include "EssClient.h"
#include "UtilsTelemetry.h"

#include <curl/curl.h>
#include <ctime>

#define CURL_TIMEOUT_SEC 10L

namespace WPEFramework
{
    namespace Plugin
    {
        EssClient::EssClient(const string &url, const string &clientId, DeviceInfoPtr deviceInfoPtr) : mUrl(url),
                                                                                                       mDeviceInfoPtr(std::move(deviceInfoPtr)),
                                                                                                       mClientId(clientId)
        {
        }

        uint32_t EssClient::GetConsentString(const string &adUsecase, EssConsentString &result)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string header;

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

            string token;
            ret = mDeviceInfoPtr->GetServiceAccessToken(token);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get service access token: %d", ret);
                return ret;
            }

            // /v1/partners/{partnerId}/accounts/{accountId}/{adUseCase}/privacyStrings?clientId={clientId}
            string url = mUrl + "/v1/partners/" + partnerId + "/accounts/" + accountId + "/" + adUsecase + "/privacyStrings?allowAdTargeting=true&allowSPI=true&clientId=" + mClientId;
            ret = ProcessGet(url, response, header);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get consent string: %d", ret);
                return ret;
            }

            return StringToConsentString(response, header, result);
        }

        uint32_t EssClient::GetPrivacySetting(const string &settingName, EssPrivacySettingData &settingData)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string header;

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

            // /v1/partners/{partnerId}/accounts/{accountId}/privacySettings?clientId={clientId}&settingFilter={settingName}
            string url = mUrl + "/v1/partners/" + partnerId + "/accounts/" + accountId + "/privacySettings?clientId=" + mClientId + "&settingFilter=" + settingName;
            ret = ProcessGet(url, response, header);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get privacy settings: %d", ret);
                return ret;
            }

            ret = StringToPrivacySetting(response, settingName, settingData);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to parse privacy settings: %d", ret);
                return ret;
            }

            return ret;
        }

        uint32_t EssClient::GetPrivacySettings(EssPrivacySettings &settings)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string header;

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

            // /v1/partners/{partnerId}/accounts/{accountId}/privacySettings?clientId={clientId}
            string url = mUrl + "/v1/partners/" + partnerId + "/accounts/" + accountId + "/privacySettings?clientId=" + mClientId;
            ret = ProcessGet(url, response, header);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get privacy settings: %d", ret);
                return ret;
            }

            ret = StringToPrivacySettings(response, settings);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to parse privacy settings: %d", ret);
                return ret;
            }

            return ret;
        }

        uint32_t EssClient::SetPrivacySettings(const EssPrivacySettings &settings)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;

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

            // /v1/partners/{partnerId}/accounts/{accountId}/privacySettings?clientId={clientId}
            string url = mUrl + "/v1/partners/" + partnerId + "/accounts/" + accountId + "/privacySettings?clientId=" + mClientId;
            string data;
            PrivacySettingsToString(settings, data);
            ret = ProcessPut(url, data);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to set privacy settings: %d", ret);
                return ret;
            }

            return ret;
        }

        uint32_t EssClient::GetExclusionPolicies(EssExclusionPolicies &policies)
        {
            uint32_t ret = Core::ERROR_NONE;
            string response;
            string header;

            string partnerId;
            ret = mDeviceInfoPtr->GetPartnerId(partnerId);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get partnerId: %d", ret);
                return ret;
            }

            // /v1/partners/{partnerId}/privacySettings/policy/usageDataExclusions?clientId={clientId}
            string url = mUrl + "/v1/partners/" + partnerId + "/privacySettings/policy/usageDataExclusions?clientId=" + mClientId;
            ret = ProcessGet(url, response, header);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get exclusion policies: %d", ret);
                return ret;
            }

            return StringToExclusionPolicies(response, policies);
        }

        static size_t CurlWriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
        {
            ((std::string *)userp)->append((char *)contents, size * nmemb);
            return size * nmemb;
        }

        long EssClient::HttpGet(const string &url, const string &authToken, std::string &response, std::string &header)
        {
            CURL *curl;
            CURLcode res;
            long retHttpCode = 0;

            if (url.empty())
            {
                LOGERR("Invalid parameters");
                return retHttpCode;
            }

            curl = curl_easy_init();
            if (!curl)
            {
                LOGERR("Failed to initialize curl");
                return retHttpCode;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);

            // Create a linked list of custom headers
            struct curl_slist *headers = NULL;
            std::string authorizationParam = "Authorization: Bearer " + authToken;
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            headers = curl_slist_append(headers, authorizationParam.c_str());

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, CurlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_HEADERDATA, &header);

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK)
            {
                LOGERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retHttpCode);
            }

            // Clean up the header list
            curl_slist_free_all(headers);

            // Clean up curl session
            curl_easy_cleanup(curl);

            return retHttpCode;
        }

        long EssClient::HttpPut(const string &url, const string &authToken, const string &data, std::string &response)
        {
            CURL *curl;
            CURLcode res;
            long retHttpCode = 0;

            if (url.empty())
            {
                LOGERR("Invalid parameters");
                return retHttpCode;
            }

            curl = curl_easy_init();
            if (!curl)
            {
                LOGERR("Failed to initialize curl");
                return retHttpCode;
            }

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, CURL_TIMEOUT_SEC);

            // Create a linked list of custom headers
            struct curl_slist *headers = NULL;
            std::string authorizationParam = "Authorization: Bearer " + authToken;
            headers = curl_slist_append(headers, "Content-Type: application/json");
            headers = curl_slist_append(headers, "Accept: application/json");
            headers = curl_slist_append(headers, "charsets: utf-8");
            headers = curl_slist_append(headers, authorizationParam.c_str());

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

            curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, data.size());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

            // Perform the request, res will get the return code
            res = curl_easy_perform(curl);

            // Check for errors
            if (res != CURLE_OK)
            {
                LOGERR("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            }
            else
            {
                curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &retHttpCode);
            }

            // Clean up the header list
            curl_slist_free_all(headers);

            // Clean up curl session
            curl_easy_cleanup(curl);

            return retHttpCode;
        }

        uint32_t EssClient::ProcessGet(const string &url, std::string &response, std::string &header)
        {
            string token;
            uint32_t ret = mDeviceInfoPtr->GetServiceAccessToken(token);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get service access token: %d", ret);
                return ret;
            }

            long code = HttpGet(url, token, response, header);
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
                code = HttpGet(url, token, response, header);
            }

            if (code != 200)
            {
                LOGERR("Failed to process GET: %ld resp: %s", code, response.c_str());
                Utils::Telemetry::SendError("Privacy_EssClient: Failed to process GET: %ld resp: %s", code, response.c_str());
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

        uint32_t EssClient::ProcessPut(const string &url, const string &data)
        {
            string token;
            uint32_t ret = mDeviceInfoPtr->GetServiceAccessToken(token);
            if (ret != Core::ERROR_NONE)
            {
                LOGERR("Failed to get service access token: %d", ret);
                return ret;
            }

            string response;
            long code = HttpPut(url, token, data, response);
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
                code = HttpPut(url, token, data, response);
            }

            if (code != 200)
            {
                LOGERR("Failed to process PUT: %ld resp: %s", code, response.c_str());
                Utils::Telemetry::SendError("Privacy_EssClient: Failed to process PUT: %ld resp: %s", code, response.c_str());
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

        uint32_t EssClient::StringToConsentString(const string &str, const string &header, EssConsentString &consentString)
        {
            // Parse the str to json object
            JsonObject json;
            if (!json.FromString(str))
            {
                LOGERR("Failed to parse string to json object: %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            if (!json.HasLabel("privacyStrings"))
            {
                LOGERR("privacyStrings not found in %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            JsonObject privacyStrings = json["privacyStrings"].Object();
            if (!privacyStrings.HasLabel("gpp"))
            {
                LOGERR("gpp not found in privacyStrings in %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            JsonObject gpp = privacyStrings["gpp"].Object();
            if (!gpp.HasLabel("value") || !gpp.HasLabel("gpp_sid"))
            {
                LOGERR("value, gpp_sid or decoded not found in gpp: %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            consentString.gppValue = gpp["value"].String();
            consentString.gppSid = gpp["gpp_sid"].String();

            if (gpp.HasLabel("decoded") && gpp["decoded"].Content() == WPEFramework::Core::JSON::Variant::type::OBJECT)
            {
                JsonObject decodedObj = gpp["decoded"].Object();
                JsonArray decodedArray;

                auto iter = decodedObj.Variants();

                while (iter.Next())
                {
                    if (iter.Current().Content() == WPEFramework::Core::JSON::Variant::type::STRING)
                    {
                        JsonObject item;
                        item[iter.Label()] = iter.Current().String();
                        decodedArray.Add(item);
                    }
                }

                if (decodedArray.Length() > 0)
                {
                    decodedArray.ToString(consentString.decoded);
                }
            }


            // Parse the header to get the optional expires time from Cache-Control: max-age=
            if (header.find("cache-control: max-age=") != std::string::npos)
            {
                std::string maxAgeStr = header.substr(header.find("cache-control: max-age=") + 23);
                maxAgeStr = maxAgeStr.substr(0, maxAgeStr.find("\r\n"));
                try
                {
                    consentString.expireTime = std::stoi(maxAgeStr);
                    LOGINFO("Got expires time from 'cache-control: max-age=' %d", consentString.expireTime);
                }
                catch (...)
                {
                    LOGERR("Invalid argument for max-age: %s", maxAgeStr.c_str());
                }
            }
            else if (header.find("Cache-Control: max-age=") != std::string::npos)
            {
                std::string maxAgeStr = header.substr(header.find("Cache-Control: max-age=") + 23);
                maxAgeStr = maxAgeStr.substr(0, maxAgeStr.find("\r\n"));
                try
                {
                    consentString.expireTime = std::stoi(maxAgeStr);
                    LOGINFO("Got expires time from 'Cache-Control: max-age=' %d", consentString.expireTime);
                }
                catch (...)
                {
                    LOGERR("Invalid argument for max-age: %s", maxAgeStr.c_str());
                }
            }

            return Core::ERROR_NONE;
        }

        uint32_t EssClient::StringToPrivacySettings(const string &str, EssPrivacySettings &settings)
        {
            std::map<const string, EssPrivacySettingData &> settingsMap =
                {
                    {EssPrivacySettingContinueWatching, settings.continueWatching},
                    {EssPrivacySettingProductAnalytics, settings.productAnalytics},
                    {EssPrivacySettingWatchHistory, settings.watchHistory},
                    {EssPrivacySettingAcrCollection, settings.acrCollection},
                    {EssPrivacySettingAppContentAdTargeting, settings.appContentAdTargeting},
                    {EssPrivacySettingCameraAnalytics, settings.cameraAnalytics},
                    {EssPrivacySettingPersonalization, settings.personalization},
                    {EssPrivacySettingPrimaryBrowseAdTargeting, settings.primaryBrowseAdTargeting},
                    {EssPrivacySettingPrimaryContentAdTargeting, settings.primaryContentAdTargeting},
                    {EssPrivacySettingRemoteDiagnostics, settings.remoteDiagnostics},
                    {EssPrivacySettingUnentitledPersonalization, settings.unentitledPersonalization},
                    {EssPrivacySettingUnentitledContinueWatching, settings.unentitledContinueWatching}
                };

            // Parse the str to json object
            JsonObject json;
            if (!json.FromString(str))
            {
                LOGERR("Failed to parse string to json object: %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            if (!json.HasLabel("settings") || json["settings"].Content() != WPEFramework::Core::JSON::Variant::type::OBJECT)
            {
                LOGERR("settings not found in %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            JsonObject settingsObj = json["settings"].Object();
            for (auto &setting : settingsMap)
            {
                if (settingsObj.HasLabel(setting.first.c_str()) && settingsObj[setting.first.c_str()].Content() == WPEFramework::Core::JSON::Variant::type::OBJECT)
                {
                    JsonObject settingObj = settingsObj[setting.first.c_str()].Object();
                    ParsePrivacySettingJsonObject(settingObj, setting.second);
                }
                else
                {
                    setting.second.available = false;
                }
            }

            return Core::ERROR_NONE;
        };

        uint32_t EssClient::StringToPrivacySetting(const string &str, const string &settingName, EssPrivacySettingData &settingData)
        {
            settingData.available = false;

            // Parse the str to json object
            JsonObject json;
            if (!json.FromString(str))
            {
                LOGERR("Failed to parse string to json object: %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            if (!json.HasLabel("settings") || json["settings"].Content() != WPEFramework::Core::JSON::Variant::type::OBJECT)
            {
                LOGERR("settings not found in %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            JsonObject settingsObj = json["settings"].Object();

            if (!settingsObj.HasLabel(settingName.c_str()) || settingsObj[settingName.c_str()].Content() != WPEFramework::Core::JSON::Variant::type::OBJECT)
            {
                LOGERR("%s not found in settings", settingName.c_str());
                return Core::ERROR_GENERAL;
            }

            JsonObject settingObj = settingsObj[settingName.c_str()].Object();

            return ParsePrivacySettingJsonObject(settingObj, settingData);
        }

        uint32_t EssClient::ParsePrivacySettingJsonObject(const JsonObject &json, EssPrivacySettingData &settingData)
        {
            if (!json.HasLabel("allowed") || json["allowed"].Content() != WPEFramework::Core::JSON::Variant::type::BOOLEAN)
            {
                LOGERR("'allowed' not found");
                return Core::ERROR_GENERAL;
            }
            settingData.allowed = json["allowed"].Boolean();

            if (json.HasLabel("expiration") && json["expiration"].Content() == WPEFramework::Core::JSON::Variant::type::STRING)
            {
                settingData.expiration = json["expiration"].String();
            }
            if (json.HasLabel("updated") && json["updated"].Content() == WPEFramework::Core::JSON::Variant::type::STRING)
            {
                settingData.updated = json["updated"].String();
            }
            if (json.HasLabel("ownerReference") && json["ownerReference"].Content() == WPEFramework::Core::JSON::Variant::type::STRING)
            {
                settingData.ownerReference = json["ownerReference"].String();
            }
            if (json.HasLabel("entityReference") && json["entityReference"].Content() == WPEFramework::Core::JSON::Variant::type::STRING)
            {
                settingData.entityReference = json["entityReference"].String();
            }
            if (json.HasLabel("isDefault") && json["isDefault"].Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN)
            {
                settingData.isDefault = json["isDefault"].Boolean();
            }
            settingData.available = true;

            return Core::ERROR_NONE;
        }

        void EssClient::PrivacySettingsToString(const EssPrivacySettings &setting, string &str)
        {
            JsonObject json;
            std::map<string, EssPrivacySettingData> settingsMap =
                {
                    {EssPrivacySettingContinueWatching, setting.continueWatching},
                    {EssPrivacySettingProductAnalytics, setting.productAnalytics},
                    {EssPrivacySettingWatchHistory, setting.watchHistory},
                    {EssPrivacySettingAcrCollection, setting.acrCollection},
                    {EssPrivacySettingAppContentAdTargeting, setting.appContentAdTargeting},
                    {EssPrivacySettingCameraAnalytics, setting.cameraAnalytics},
                    {EssPrivacySettingPersonalization, setting.personalization},
                    {EssPrivacySettingPrimaryBrowseAdTargeting, setting.primaryBrowseAdTargeting},
                    {EssPrivacySettingPrimaryContentAdTargeting, setting.primaryContentAdTargeting},
                    {EssPrivacySettingRemoteDiagnostics, setting.remoteDiagnostics},
                    {EssPrivacySettingUnentitledPersonalization, setting.unentitledPersonalization},
                    {EssPrivacySettingUnentitledContinueWatching, setting.unentitledContinueWatching}
                };

            for (auto &s : settingsMap)
            {
                if (s.second.available)
                {
                    JsonObject settingObj;
                    settingObj["allowed"] = s.second.allowed;
                    settingObj["ownerReference"] = s.second.ownerReference;
                    json[s.first.c_str()] = settingObj;
                }
            }

            json.ToString(str);
        }

        uint32_t EssClient::StringToExclusionPolicies(const string &str, EssExclusionPolicies &policies)
        {
            std::map<string, EssExclusionPolicyData &> policiesMap =
                {
                    {EssExclusionPolicyProductAnalytics, policies.productAnalytics},
                    {EssExclusionPolicyBusinessAnalytics, policies.businessAnalytics},
                    {EssExclusionPolicyPersonalization, policies.personalization}
                };

            for (auto &policy : policiesMap)
            {
                policy.second.available = false;
            }

            // Parse the str to json object
            JsonObject json;
            if (!json.FromString(str))
            {
                LOGERR("Failed to parse string to json object: %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            if (!json.HasLabel("exclusionPolicy") || json["exclusionPolicy"].Content() != WPEFramework::Core::JSON::Variant::type::OBJECT)
            {
                LOGERR("exclusionPolicy not found in %s", str.c_str());
                return Core::ERROR_GENERAL;
            }

            JsonObject exclusionPolicy = json["exclusionPolicy"].Object();

            auto processPolicy = [](const JsonObject &json, const char *label, EssExclusionPolicyData &policyData)
            {
                if (json.HasLabel(label) && json[label].Content() == WPEFramework::Core::JSON::Variant::type::OBJECT)
                {
                    JsonObject policyObj = json[label].Object();

                    if (policyObj.HasLabel("dataEvents") && policyObj["dataEvents"].Content() == WPEFramework::Core::JSON::Variant::type::ARRAY)
                    {
                        JsonArray dataEventsArray = policyObj["dataEvents"].Array();
                        for (int i = 0; i < dataEventsArray.Length(); ++i)
                        {
                            policyData.dataEvents.insert(dataEventsArray[i].String());
                        }
                    }

                    if (policyObj.HasLabel("entityReference") && policyObj["entityReference"].Content() == WPEFramework::Core::JSON::Variant::type::ARRAY)
                    {
                        JsonArray entityReferenceArray = policyObj["entityReference"].Array();
                        for (int i = 0; i < entityReferenceArray.Length(); ++i)
                        {
                            policyData.entityReference.insert(entityReferenceArray[i].String());
                        }
                    }

                    if (policyObj.HasLabel("derivativePropagation") && policyObj["derivativePropagation"].Content() == WPEFramework::Core::JSON::Variant::type::BOOLEAN)
                    {
                        policyData.derivativePropagation = policyObj["derivativePropagation"].Boolean();
                    }

                    policyData.available = true;
                }
            };

            for (auto &policy : policiesMap)
            {
                processPolicy(exclusionPolicy, policy.first.c_str(), policy.second);
            }

            return Core::ERROR_NONE;
        }

    } // namespace Plugin
} // namespace WPEFramework
