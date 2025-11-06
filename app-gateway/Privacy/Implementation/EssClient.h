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
#pragma once

#include "../Module.h"
#include "EssTypes.h"
#include "DeviceInfo.h"
#include "utils.h"

#include <map>
#include <set>
#include <list>

namespace WPEFramework {
namespace Plugin {

    class EssClient {
    public:
        EssClient(const string &url, const string &clientId, DeviceInfoPtr deviceInfoPtr);
        ~EssClient() = default;

        uint32_t GetConsentString(const string &adUsecase, EssConsentString &result);
        uint32_t GetPrivacySetting(const string &settingName, EssPrivacySettingData &settingData);
        uint32_t GetPrivacySettings(EssPrivacySettings &settings);
        uint32_t SetPrivacySettings(const EssPrivacySettings &settings);
        uint32_t GetExclusionPolicies(EssExclusionPolicies &policies);

    private:
        long HttpGet(const string &url, const string &authToken, std::string &response, std::string &header);
        long HttpPut(const string &url, const string &authToken, const string &data, std::string &response);
        uint32_t ProcessGet(const string &url, std::string &response, std::string &header);
        uint32_t ProcessPut(const string &url, const string &data);
        uint32_t StringToConsentString(const string &str, const string &header, EssConsentString &consentString);
        uint32_t StringToPrivacySettings(const string &str, EssPrivacySettings &settings);
        uint32_t StringToPrivacySetting(const string &str, const string &settingName, EssPrivacySettingData &settingData);
        void PrivacySettingsToString(const EssPrivacySettings &setting, string &str);
        uint32_t StringToExclusionPolicies(const string &str, EssExclusionPolicies &policies);
        uint32_t ParsePrivacySettingJsonObject(const JsonObject &json, EssPrivacySettingData &settingData);

        std::string mUrl;
        DeviceInfoPtr mDeviceInfoPtr;

        std::string mClientId;
    };

    typedef std::shared_ptr<EssClient> EssClientPtr;

    } // namespace Plugin
} // namespace WPEFramework




