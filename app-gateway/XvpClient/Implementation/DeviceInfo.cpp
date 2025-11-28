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
#include "UtilsLogging.h"
#include "UtilsJsonrpcDirectLink.h"

#define AUTHSERVICE_CALLSIGN "org.rdk.AuthService"

namespace WPEFramework
{
    namespace Plugin
    {

        static std::string sThunderSecurityToken;

        DeviceInfo::DeviceInfo(PluginHost::IShell *shell) : mShell(shell),
                                                            mMutex(),
                                                            mInitializationThread(),
                                                            mPartnerId(),
                                                            mDeviceId()
        {
            ASSERT(mShell != nullptr);
            mShell->AddRef();
            mInitializationThread = std::thread(&DeviceInfo::Initialize, this);
        }

        DeviceInfo::~DeviceInfo()
        {
            LOGINFO("~DeviceInfo destructor");
            if (mInitializationThread.joinable())
            {
                mInitializationThread.join();
            }

            std::unique_lock<std::mutex> lock(mMutex);
            if (mShell != nullptr)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        uint32_t DeviceInfo::GetPartnerId(std::string &partnerId)
        {
            uint32_t result = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);
            if (mPartnerId.empty())
            {
                JsonObject params;
                JsonObject response;
                auto serviceLink = Utils::GetThunderControllerClient(mShell, AUTHSERVICE_CALLSIGN);
                if (serviceLink == nullptr)
                {
                    return Core::ERROR_UNAVAILABLE;
                }

                result = serviceLink->Invoke<JsonObject, JsonObject>("getDeviceId", params, response);
                if (result == Core::ERROR_NONE && response.HasLabel("partnerId"))
                {
                    mPartnerId = response["partnerId"].String();
                    partnerId = mPartnerId;
                }
                else
                {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }
            else
            {
                partnerId = mPartnerId;
            }
            return result;
        }

        uint32_t DeviceInfo::GetAccountId(std::string &accountId)
        {
            uint32_t result = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);
            JsonObject params;
            JsonObject response;
            auto serviceLink = Utils::GetThunderControllerClient(mShell, AUTHSERVICE_CALLSIGN);
            if (serviceLink == nullptr)
            {
                return Core::ERROR_UNAVAILABLE;
            }

            result = serviceLink->Invoke<JsonObject, JsonObject>("getServiceAccountId", params, response);
            if (result == Core::ERROR_NONE && response.HasLabel("serviceAccountId"))
            {
                accountId = response["serviceAccountId"].String();
                if (accountId.empty())
                {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }
            else
            {
                result = Core::ERROR_UNAVAILABLE;
            }

            return result;
        }

        uint32_t DeviceInfo::GetServiceAccessToken(std::string &token)
        {
            uint32_t result = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);

            JsonObject params;
            JsonObject response;

            auto serviceLink = Utils::GetThunderControllerClient(mShell, AUTHSERVICE_CALLSIGN);
            if (serviceLink == nullptr)
            {
                return Core::ERROR_UNAVAILABLE;
            }

            result = serviceLink->Invoke<JsonObject, JsonObject>("getServiceAccessToken", params, response);
            if (result == Core::ERROR_NONE && response.HasLabel("token") && response.HasLabel("expires"))
            {
                token = response["token"].String();
            }
            else
            {
                result = Core::ERROR_UNAVAILABLE;
            }
            return result;
        }

        uint32_t DeviceInfo::GetDeviceId(std::string &deviceId)
        {
            uint32_t result = Core::ERROR_NONE;
            std::unique_lock<std::mutex> lock(mMutex);
            if (mDeviceId.empty())
            {
                JsonObject params;
                JsonObject response;
                auto serviceLink = Utils::GetThunderControllerClient(mShell, AUTHSERVICE_CALLSIGN);
                if (serviceLink == nullptr)
                {
                    return Core::ERROR_UNAVAILABLE;
                }

                result = serviceLink->Invoke<JsonObject, JsonObject>("getXDeviceId", params, response);
                if (result == Core::ERROR_NONE && response.HasLabel("xDeviceId"))
                {
                    mDeviceId = response["xDeviceId"].String();
                    deviceId = mDeviceId;
                }
                else
                {
                    result = Core::ERROR_UNAVAILABLE;
                }
            }
            else
            {
                deviceId = mDeviceId;
            }

            return result;
        }

        void DeviceInfo::Initialize()
        {
            std::unique_lock<std::mutex> lock(mMutex);
            auto serviceLink = Utils::GetThunderControllerClient(mShell, AUTHSERVICE_CALLSIGN);
            if (serviceLink)
            {
                JsonObject params;
                JsonObject response;

                // Get partnerId from AuthService.getDeviceId
                uint32_t result = serviceLink->Invoke<JsonObject, JsonObject>("getDeviceId", params, response);
                if (result == Core::ERROR_NONE && response.HasLabel("partnerId"))
                {
                    mPartnerId = response["partnerId"].String();
                    LOGINFO("Got partnerId %s", mPartnerId.c_str());
                }
                else
                {
                    LOGERR("Failed to get partnerId: %d", result);
                }

                result = serviceLink->Invoke<JsonObject, JsonObject>("getXDeviceId", params, response);
                if (result == Core::ERROR_NONE && response.HasLabel("xDeviceId"))
                {
                    mDeviceId = response["xDeviceId"].String();
                    LOGINFO("Got xDeviceId %s", mDeviceId.c_str());
                }
                else
                {
                    LOGERR("Failed to get xDeviceId: %d", result);
                }
            }
            else
            {
                LOGERR("Failed to get AuthService link");
            }
        }

    } // namespace Plugin
} // namespace WPEFramework
