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
#include "utils.h"

#include <interfaces/IAuthService.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace WPEFramework {
namespace Plugin {

    class DeviceInfo {
    public:
        DeviceInfo(PluginHost::IShell *shell);
        ~DeviceInfo();

        DeviceInfo(const DeviceInfo&) = delete;
        DeviceInfo& operator=(const DeviceInfo&) = delete;

        uint32_t GetPartnerId(std::string &partnerId);
        uint32_t GetAccountId(std::string &accountId);
        uint32_t GetServiceAccessToken(std::string &token);
        void OnServiceAccountIdChanged(const std::string &newServiceAccountId);
        void OnServiceAccessTokenChanged();

    private:

        void ActionLoop();

        class AuthServiceNotification : public Exchange::IAuthService::INotification {
        public:
            AuthServiceNotification() = delete;
            AuthServiceNotification(const AuthServiceNotification &) = delete;
            AuthServiceNotification &operator=(const AuthServiceNotification &) = delete;

            AuthServiceNotification(DeviceInfo *parent): mParent(*parent)
            {
            }
            ~AuthServiceNotification() override
            {
            }

            BEGIN_INTERFACE_MAP(Notification)
            INTERFACE_ENTRY(Exchange::IAuthService::INotification)
            END_INTERFACE_MAP

            void OnActivationStatusChanged(const std::string &oldActivationStatus, const std::string &newActivationStatus) override
            {
            }
            void OnServiceAccountIdChanged(const std::string &oldServiceAccountId, const std::string &newServiceAccountId) override
            {
                mParent.OnServiceAccountIdChanged(newServiceAccountId);
            }
            void AuthTokenChanged() override
            {
            }
            void SessionTokenChanged() override
            {
            }
            void ServiceAccessTokenChanged() override
            {
                mParent.OnServiceAccessTokenChanged();
            }
            void OnPartnerIdChanged(const string& oldPartnerId, const string& newPartnerId) override
            {
                // Privacy plugin doesn't need to handle partner ID changes
            }

        private:
            DeviceInfo &mParent;
        };

        enum ActionType {
            ACTION_TYPE_SERVICE_ACCOUNT_ID_CHANGED,
            ACTION_TYPE_SERVICE_ACCESS_TOKEN_CHANGED,
            ACTION_TYPE_SHUTDOWN,
            ACTION_TYPE_NO_ACTION
        };

        struct Action {
            ActionType type;
            std::string value;
        };   
        
        PluginHost::IShell *mShell;
        Core::Sink<AuthServiceNotification> mAuthServiceNotification;

        std::mutex mQueueMutex;
        std::condition_variable mQueueCondition;
        std::thread mThread;
        std::queue<Action> mActionQueue;

        std::mutex mMutex;
        string mPartnerId;
        string mAccountId;
        string mServiceAccessToken;
    };

    typedef std::shared_ptr<DeviceInfo> DeviceInfoPtr;

} // namespace Plugin
} // namespace WPEFramework
