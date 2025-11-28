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
#include "FbDiscoveryImplementation.h"
#include "UtilsLogging.h"

#include <interfaces/IXvpClient.h>


#define XVP_CLIENT_CALLSIGN  "org.rdk.XvpClient"

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(FbDiscoveryImplementation, 1, 0);

        FbDiscoveryImplementation::FbDiscoveryImplementation() : mShell(nullptr)
        {
        }

        FbDiscoveryImplementation::~FbDiscoveryImplementation()
        {
            // Cleanup resources if needed
            if (mShell != nullptr)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        uint32_t FbDiscoveryImplementation::Configure(PluginHost::IShell *shell)
        {
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();

            return Core::ERROR_NONE;
        }

        Core::hresult FbDiscoveryImplementation::ClearContentAccess(const Exchange::IFbDiscovery::Context& context)
        {
            LOGINFO("ClearContentAccess");
            Core::hresult result = Core::ERROR_GENERAL;
            if (mShell == nullptr)
            {
                LOGERR("mShell is null");
                return result;
            }

            auto xvpSession = mShell->QueryInterfaceByCallsign<Exchange::IXvpSession>(XVP_CLIENT_CALLSIGN);
            if (xvpSession != nullptr)
            {
                result = xvpSession->ClearContentAccess(context.appId);
                xvpSession->Release();
            }
            return result;
        }

        Core::hresult FbDiscoveryImplementation::ContentAccess(const Exchange::IFbDiscovery::Context& context, const string& ids)
        {
            LOGINFO("ContentAccess for appId: %s, ids: %s", context.appId.c_str(), ids.c_str());
            Core::hresult result = Core::ERROR_GENERAL;
            if (mShell == nullptr)
            {
                LOGERR("mShell is null");
                return Core::ERROR_GENERAL;
            }

            // Get 'availabilities' and 'entitlements' from ids
            JsonObject idsJson;
            idsJson.FromString(ids);
            string availabilities;
            string entitlements;
            if (idsJson.HasLabel("availabilities") && idsJson["availabilities"].Content() == Core::JSON::Variant::type::ARRAY)
            {
                JsonArray availabilitiesArray = idsJson["availabilities"].Array();
                availabilitiesArray.ToString(availabilities);
            }

            if (idsJson.HasLabel("entitlements") && idsJson["entitlements"].Content() == Core::JSON::Variant::type::ARRAY)
            {
                JsonArray entitlementsArray = idsJson["entitlements"].Array();
                entitlementsArray.ToString(entitlements);
            }

            auto xvpSession = mShell->QueryInterfaceByCallsign<Exchange::IXvpSession>(XVP_CLIENT_CALLSIGN);
            if (xvpSession != nullptr)
            {
                result = xvpSession->SetContentAccess(context.appId, availabilities, entitlements);
                xvpSession->Release();
            }
            else
            {
                LOGERR("Failed to get XVP session");
            }
            return result;
        }

        Core::hresult FbDiscoveryImplementation::SignIn(const Exchange::IFbDiscovery::Context& context, const string& entitlements, bool& success)
        {
            LOGINFO("SignIn");
            Core::hresult result = Core::ERROR_NONE;

            if (mShell == nullptr)
            {
                LOGERR("mShell is null");
                return Core::ERROR_GENERAL;
            }

            // Check if entitlements are provided
            if (entitlements.empty() == false)
            {
                // Check if array is not empty
                JsonArray entitlementsJson;
                entitlementsJson.FromString(entitlements);
                if (entitlementsJson.Length() > 0)
                {
                    auto xvpSession = mShell->QueryInterfaceByCallsign<Exchange::IXvpSession>(XVP_CLIENT_CALLSIGN);
                    if (xvpSession != nullptr)
                    {
                        result = xvpSession->SetContentAccess(context.appId, "", entitlements);
                        xvpSession->Release();
                    }
                    else
                    {
                        LOGERR("Failed to get XVP session");
                        result = Core::ERROR_GENERAL;
                    }
                }
            }

            if (result == Core::ERROR_NONE)
            {
                auto xvpVideo = mShell->QueryInterfaceByCallsign<Exchange::IXvpVideo>(XVP_CLIENT_CALLSIGN);
                if (xvpVideo != nullptr)
                {
                    result = xvpVideo->SignIn(context.appId, true);
                    xvpVideo->Release();
                }
                else
                {
                    LOGERR("Failed to get XVP session");
                    result = Core::ERROR_GENERAL;
                }
            }

            if (result != Core::ERROR_NONE)
            {
                LOGERR("SignIn failed: %d", result);
            }

            success = (result == Core::ERROR_NONE);
            return result;
        }

        Core::hresult FbDiscoveryImplementation::SignOut(const Exchange::IFbDiscovery::Context& context, bool& success)
        {
            LOGINFO("SignOut");
            Core::hresult result = Core::ERROR_NONE;
            if (mShell == nullptr)
            {
                LOGERR("mShell is null");
                return Core::ERROR_GENERAL;
            }

            auto xvpVideo = mShell->QueryInterfaceByCallsign<Exchange::IXvpVideo>(XVP_CLIENT_CALLSIGN);
            if (xvpVideo != nullptr)
            {
                result = xvpVideo->SignIn(context.appId, false);
                xvpVideo->Release();
            }
            else
            {
                LOGERR("Failed to get XVP session");
                result = Core::ERROR_GENERAL;
            }

            if (result != Core::ERROR_NONE)
            {
                LOGERR("SignOut failed: %d", result);
            }

            success = (result == Core::ERROR_NONE);

            return result;
        }

        Core::hresult FbDiscoveryImplementation::Watched(const Exchange::IFbDiscovery::Context& context,
            const string& entityId, const double progress, const bool completed, const string& watchedOn, bool& success)
        {
            LOGINFO("Watched");
            Core::hresult result = Core::ERROR_NONE;
            if (mShell == nullptr)
            {
                LOGERR("mShell is null");
                return Core::ERROR_GENERAL;
            }

            auto xvpPlayback = mShell->QueryInterfaceByCallsign<Exchange::IXvpPlayback>(XVP_CLIENT_CALLSIGN);
            if (xvpPlayback != nullptr)
            {
                result = xvpPlayback->PutResumePoint(context.appId, entityId, progress, completed, watchedOn);
                xvpPlayback->Release();
            }
            else
            {
                LOGERR("Failed to get XVP session");
                result = Core::ERROR_GENERAL;
            }

            if (result != Core::ERROR_NONE)
            {
                LOGERR("Watched failed: %d", result);
            }

            success = (result == Core::ERROR_NONE);


            return result;
        }

        Core::hresult FbDiscoveryImplementation::WatchNext(const Exchange::IFbDiscovery::Context& context,
                const string& title, const string& identifiers, const string& expires, const string& images, bool& success)
        {
            LOGINFO("WatchNext");
            if (mShell == nullptr)
            {
                LOGERR("mShell is null");
                return Core::ERROR_GENERAL;
            }

            JsonObject identifiersJson;
            string entityId;
            if (identifiersJson.FromString(identifiers) == false)
            {
                LOGERR("Failed to parse identifiers");
                success = false;
                return Core::ERROR_GENERAL;
            }

            if (identifiersJson.HasLabel("entityId") && identifiersJson["entityId"].Content() == WPEFramework::Core::JSON::Variant::type::STRING)
            {
                entityId = identifiersJson["entityId"].String();
                LOGINFO("entityId: %s", entityId.c_str());
            }
            else
            {
                LOGERR("No entityId in identifiers");
                success = false;
                return Core::ERROR_GENERAL;
            }

            return Watched(context, entityId, 1.0, false, "", success);
        } 

    } // namespace Plugin
} // namespace WPEFramework
