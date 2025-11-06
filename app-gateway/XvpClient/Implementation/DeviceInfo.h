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

#include "Module.h"

#include <mutex>

namespace WPEFramework
{
    namespace Plugin
    {

        class DeviceInfo
        {
        public:
            DeviceInfo(PluginHost::IShell *shell);
            ~DeviceInfo();

            DeviceInfo(const DeviceInfo &) = delete;
            DeviceInfo &operator=(const DeviceInfo &) = delete;

            uint32_t GetPartnerId(std::string &partnerId);
            uint32_t GetAccountId(std::string &accountId);
            uint32_t GetServiceAccessToken(std::string &token);
            uint32_t GetDeviceId(std::string &deviceId);

        private:
            void Initialize();

            PluginHost::IShell *mShell;

            std::mutex mMutex;
            std::thread mInitializationThread;
            std::string mPartnerId;
            std::string mDeviceId;
        };

        typedef std::shared_ptr<DeviceInfo> DeviceInfoPtr;

    } // namespace Plugin
} // namespace WPEFramework
