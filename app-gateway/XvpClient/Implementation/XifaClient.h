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
#include "DeviceInfo.h"
#include <memory>

namespace WPEFramework {
namespace Plugin {

    class XifaClient {
    public:
        XifaClient(PluginHost::IShell* shell, const string &host, const string &clientId, DeviceInfoPtr deviceInfoPtr);
        ~XifaClient();

        uint32_t GetAdIdentifier(const string &appId, const string &scopeType, const std::string &scopeId,
                string &ifa, string &ifa_type, string &lmt);
        uint32_t ResetAdIdentifier(const string &appId, const string &scopeType, const std::string &scopeId);

        private:
        uint32_t ProcessGet(const string &url, std::string &response, std::string &responseHeader);
        uint32_t ProcessPost(const string &url, const string &data, string &response, string &responseHeader);

        std::string mHost;
        std::string mClientId;
        DeviceInfoPtr mDeviceInfoPtr;
        PluginHost::IShell* mShell;
        std::string mThunderSecurityToken;
    };

    using XifaClientPtr = std::shared_ptr<XifaClient>;
}
}