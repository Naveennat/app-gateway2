/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#include "DeviceInfo.h"

#define AUTHSERVICE_CALLSIGN "org.rdk.AuthService"

namespace WPEFramework {
namespace Plugin {

        DeviceInfo::DeviceInfo(PluginHost::IShell *shell):
            mShell(shell),
            mAuthServiceNotification(this),
            mQueueMutex(),
            mQueueCondition(),
            mActionQueue(),
            mMutex(),
            mPartnerId(),
            mAccountId(),
            mServiceAccessToken()
        {
            ASSERT(mShell != nullptr);
            mShell->AddRef();
            mThread =  std::thread(&DeviceInfo::ActionLoop, this);
        }

        DeviceInfo::~DeviceInfo()
        {
            auto interface = mShell->QueryInterfaceByCallsign<Exchange::IAuthService>(AUTHSERVICE_CALLSIGN);
            if (interface != nullptr)
            {
                interface->Unregister(&mAuthServiceNotification);
                interface->Release();
            }

            Action action = {ACTION_TYPE_SHUTDOWN, string()};
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
            mThread.join();

            mShell->Release();
            mShell = nullptr;
        }

        uint32_t DeviceInfo::GetPartnerId(std::string &partnerId)
        {
            uint32_t ret = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);
            if (mPartnerId.empty())
            {
                auto interface = mShell->QueryInterfaceByCallsign<Exchange::IAuthService>(AUTHSERVICE_CALLSIGN);
                if (interface != nullptr)
                {
                    Exchange::IAuthService::GetDeviceIdResult result;
                    uint32_t err = interface->GetDeviceId(result);
                    interface->Release();
                    if (err == Core::ERROR_NONE && result.success)
                    {
                        partnerId = result.partnerId;
                    }
                    else
                    {
                        LOGERR("Failed to get partnerId: %d", err);
                        ret = err;
                    }
                }
                else
                {
                    LOGERR("Failed to get AuthService interface");
                    ret = Core::ERROR_UNAVAILABLE;
                }
            }
            else
            {
                partnerId = mPartnerId;
            }

            return ret;
        }

        uint32_t DeviceInfo::GetAccountId(std::string &accountId)
        {
            uint32_t ret = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);
            if (mAccountId.empty())
            {
                // Try to get accountId from AuthService
                auto interface = mShell->QueryInterfaceByCallsign<Exchange::IAuthService>(AUTHSERVICE_CALLSIGN);
                if (interface != nullptr)
                {
                    Exchange::IAuthService::GetServiceAccountIdResult result;
                    uint32_t err = interface->GetServiceAccountId(result);
                    interface->Release();
                    if (err == Core::ERROR_NONE && result.success)
                    {
                        accountId = result.serviceAccountId;
                    }
                    else
                    {
                        LOGERR("Failed to get accountId: %d", err);
                        ret = Core::ERROR_UNAVAILABLE;
                    }
                }
                else
                {
                    LOGERR("Failed to get AuthService interface");
                    ret = Core::ERROR_UNAVAILABLE;
                }
            }
            else
            {
                accountId = mAccountId;
            }
            return ret;
        }

        uint32_t DeviceInfo::GetServiceAccessToken(std::string &token)
        {
            uint32_t ret = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);
            if (mServiceAccessToken.empty())
            {
                // Try to get serviceAccessToken from AuthService
                auto interface = mShell->QueryInterfaceByCallsign<Exchange::IAuthService>(AUTHSERVICE_CALLSIGN);
                if (interface != nullptr)
                {
                    Exchange::IAuthService::GetServiceAccessTokenResult result;
                    uint32_t err = interface->GetServiceAccessToken(result);
                    interface->Release();
                    if (err == Core::ERROR_NONE && result.success)
                    {
                        token = result.token;
                    }
                    else
                    {
                        LOGERR("Failed to get service access token: %d", err);
                        ret = Core::ERROR_UNAVAILABLE;
                    }
                }
                else
                {
                    LOGERR("Failed to get AuthService interface");
                    ret = Core::ERROR_UNAVAILABLE;
                }
            }
            else
            {
                token = mServiceAccessToken;
            }
            return ret;
        }

        void DeviceInfo::OnServiceAccountIdChanged(const std::string &newServiceAccountId)
        {
            Action action = {ACTION_TYPE_SERVICE_ACCOUNT_ID_CHANGED, newServiceAccountId};
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        }

        void DeviceInfo::OnServiceAccessTokenChanged()
        {
            Action action = {ACTION_TYPE_SERVICE_ACCESS_TOKEN_CHANGED, string()};
            {
                std::lock_guard<std::mutex> lock(mQueueMutex);
                mActionQueue.push(action);
            }
            mQueueCondition.notify_one();
        }

        void DeviceInfo::ActionLoop()
        {
            //Init
            auto interface = mShell->QueryInterfaceByCallsign<Exchange::IAuthService>(AUTHSERVICE_CALLSIGN);
            if (interface != nullptr)
            {
                interface->Register(&mAuthServiceNotification);
                //Get initial values
                std::lock_guard<std::mutex> lock(mMutex);
                Exchange::IAuthService::GetDeviceIdResult resultPartnerId;
                uint32_t err = interface->GetDeviceId(resultPartnerId);
                if (err == Core::ERROR_NONE && resultPartnerId.success)
                {
                    mPartnerId = resultPartnerId.partnerId;
                }
                else
                {
                    LOGERR("Failed to get partnerId: %d", err);
                }

                Exchange::IAuthService::GetServiceAccountIdResult resultAccountId;
                err = interface->GetServiceAccountId(resultAccountId);
                if (err == Core::ERROR_NONE && resultAccountId.success)
                {
                    mAccountId = resultAccountId.serviceAccountId;
                }
                else
                {
                    LOGERR("Failed to get accountId: %d", err);
                }

                Exchange::IAuthService::GetServiceAccessTokenResult resultAccessToken;
                err = interface->GetServiceAccessToken(resultAccessToken);
                interface->Release();
                if (err == Core::ERROR_NONE && resultAccessToken.success)
                {
                    mServiceAccessToken = resultAccessToken.token;
                }
                else
                {
                    LOGERR("Failed to get service access token: %d", err);
                }
            }
            else
            {
                LOGERR("Failed to get AuthService interface");
            }

            while (true)
            {
                Action action = {ACTION_TYPE_NO_ACTION, string()};
                {
                    std::unique_lock<std::mutex> lock(mQueueMutex);
                    if (mActionQueue.empty())
                    {
                        mQueueCondition.wait(lock, [this] { return !mActionQueue.empty(); });
                    }
                    
                    if (!mActionQueue.empty())
                    {
                        action = mActionQueue.front();
                        mActionQueue.pop();
                    }
                }

                switch (action.type)
                {
                case ACTION_TYPE_SERVICE_ACCOUNT_ID_CHANGED:
                {
                    std::lock_guard<std::mutex> lock(mMutex);
                    mAccountId = action.value;
                    LOGINFO("New ServiceAccountId: %s", action.value.c_str());
                    break;
                }
                case ACTION_TYPE_SERVICE_ACCESS_TOKEN_CHANGED:
                {
                    auto interface = mShell->QueryInterfaceByCallsign<Exchange::IAuthService>(AUTHSERVICE_CALLSIGN);
                    if (interface != nullptr)
                    {
                        Exchange::IAuthService::GetServiceAccessTokenResult result;
                        uint32_t err = interface->GetServiceAccessToken(result);
                        interface->Release();
                        if (err == Core::ERROR_NONE && result.success)
                        {
                            std::lock_guard<std::mutex> lock(mMutex);
                            mServiceAccessToken = result.token;
                            LOGINFO("New ServiceAccessToken fetched");
                        }
                        else
                        {
                            LOGERR("Failed to get service access token: %d", err);
                        }
                    }
                    else
                    {
                        LOGERR("Failed to get AuthService interface");
                    }
                    break;
                }
                case ACTION_TYPE_SHUTDOWN:
                {
                    LOGINFO("Shutting down DeviceInfo action loop");
                    return;
                }
                default:
                    break;
                }
            }
        }


} // namespace Plugin
} // namespace WPEFramework