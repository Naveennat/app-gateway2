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
#include "XvpClientImplementation.h"
#include "UtilsLogging.h"
#include "UtilsCallsign.h"
#include <interfaces/ILaunchDelegate.h>

#include <fstream>
#include <streambuf>

#define XVP_CLIENT_ID_DFL "ripple"

#define XVP_DATA_SCOPE_SIGN_IN_STATE_DFL   "account"

namespace WPEFramework
{
    namespace Plugin
    {

        class XvpClientConfig : public Core::JSON::Container
        {
        private:
            XvpClientConfig(const XvpClientConfig &) = delete;
            XvpClientConfig &operator=(const XvpClientConfig &) = delete;

            class ServiceConfig : public Core::JSON::Container
            {
            public:
                ServiceConfig()
                    : Core::JSON::Container(), Url()
                {
                    Add(_T("url"), &Url);
                }

            public:
                Core::JSON::String Url;
            };

        public:
            XvpClientConfig()
                : Core::JSON::Container(), XvpXifa(), XvpPlayback(), XvpVideo(), XvpSession()
            {
                Add(_T("xvp_xifa_service"), &XvpXifa);
                Add(_T("xvp_playback_service"), &XvpPlayback);
                Add(_T("xvp_video_service"), &XvpVideo);
                Add(_T("xvp_session_service"), &XvpSession);
            }
            ~XvpClientConfig()
            {
            }

        public:
            ServiceConfig XvpXifa;
            ServiceConfig XvpPlayback;
            ServiceConfig XvpVideo;
            ServiceConfig XvpSession;
        };

        SERVICE_REGISTRATION(XvpClientImplementation, 1, 0);

        XvpClientImplementation::XvpClientImplementation() : mShell(nullptr),
                                                             mDeviceInfoPtr(nullptr),
                                                             mXifaClientPtr(nullptr),
                                                             mPlaybackClientPtr(nullptr),
                                                             mVideoClientPtr(nullptr),
                                                             mSessionClientPtr(nullptr)
        {
            
        }

        XvpClientImplementation::~XvpClientImplementation()
        {
            LOGINFO("~XvpClientImplementation destructor");

            if (mShell != nullptr)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        uint32_t XvpClientImplementation::Configure(PluginHost::IShell *shell)
        {
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();

            // Read XvpClient config from file and parse it
            XvpClientConfig config;
            Core::OptionalType<Core::JSON::Error> error;
            std::ifstream xvpClientConfigFile(PLUGIN_PRODUCT_CFG);
            if (!xvpClientConfigFile.is_open())
            {
                LOGERR("Failed to open XvpClient config file: %s", PLUGIN_PRODUCT_CFG);
            }
            else
            {
                std::string xvpClientConfigContent((std::istreambuf_iterator<char>(xvpClientConfigFile)), std::istreambuf_iterator<char>());
                if (config.FromString(xvpClientConfigContent, error) == false)
                {
                    LOGERR("Failed to parse XvpClient config file, error: '%s', content: '%s'.",
                           (error.IsSet() ? error.Value().Message().c_str() : "Unknown"),
                           xvpClientConfigContent.c_str());
                }
            }

            mDeviceInfoPtr = std::make_shared<DeviceInfo>(mShell);

            LOGINFO("XvpXifa URL: %s", config.XvpXifa.Url.Value().c_str());
            mXifaClientPtr = std::make_shared<XifaClient>(mShell, config.XvpXifa.Url.Value(),
                                                          XVP_CLIENT_ID_DFL, mDeviceInfoPtr);
            LOGINFO("XvpPlayback URL: %s", config.XvpPlayback.Url.Value().c_str());
            mPlaybackClientPtr = std::make_shared<PlaybackClient>(mShell, config.XvpPlayback.Url.Value(),
                                                                  XVP_CLIENT_ID_DFL, mDeviceInfoPtr);
            LOGINFO("XvpVideo URL: %s", config.XvpVideo.Url.Value().c_str());
            mVideoClientPtr = std::make_shared<VideoClient>(config.XvpVideo.Url.Value(),
                                                            XVP_CLIENT_ID_DFL, mDeviceInfoPtr);
            LOGINFO("XvpSession URL: %s", config.XvpSession.Url.Value().c_str());
            mSessionClientPtr = std::make_shared<SessionClient>(config.XvpSession.Url.Value(),
                                                                XVP_CLIENT_ID_DFL, mDeviceInfoPtr);

            return Core::ERROR_NONE;
        }

        Core::hresult XvpClientImplementation::ClearContentAccess(const string &appId)
        {
            LOGINFO("ClearContentAccess for appId: %s", appId.c_str());
            if (mSessionClientPtr)
            {
                return mSessionClientPtr->ClearContentAccess(appId);
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult XvpClientImplementation::SetContentAccess(const string &appId,
                                                                const string &availabilities,
                                                                const string &entitlements)
        {
            LOGINFO("SetContentAccess for appId: %s, availabilities: %s, entitlements: %s", appId.c_str(), availabilities.c_str(), entitlements.c_str());
            if (mSessionClientPtr)
            {
                return mSessionClientPtr->SetContentAccess(appId, availabilities, entitlements);
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult XvpClientImplementation::SignIn(const string& appId, bool isSignedIn)
        {
            LOGINFO("SignIn for appId: %s, isSignedIn: %s", appId.c_str(), isSignedIn ? "true" : "false");
            if (mVideoClientPtr)
            {
                return mVideoClientPtr->SignIn(appId, XVP_DATA_SCOPE_SIGN_IN_STATE_DFL, isSignedIn);
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult XvpClientImplementation::PutResumePoint(const string &appId,
                                                                 const string &entityId,
                                                                 const double progress,
                                                                 const bool completed,
                                                                 const string &watchedOn)
        {
            LOGINFO("PutResumePoint for appId: %s, entityId: %s, progress: %f, completed: %s, watchedOn: %s",
                    appId.c_str(), entityId.c_str(), progress, completed ? "true" : "false", watchedOn.c_str());
            if (mPlaybackClientPtr)
            {
                string contentPartnerId = "";
                if (mShell != nullptr)
                {
                    auto launchDelegate = mShell->QueryInterfaceByCallsign<Exchange::ILaunchDelegate>(INTERNAL_GATEWAY_CALLSIGN);
                    if (launchDelegate != nullptr)
                    {
                        uint32_t err = launchDelegate->GetContentPartnerId(appId, contentPartnerId);
                        launchDelegate->Release();

                        if (err != Core::ERROR_NONE || contentPartnerId.empty())
                        {
                            LOGERR("Failed to get content partner ID for appId: %s, error: %d", appId.c_str(), err);
                        }
                    }
                    else
                    {
                        LOGERR("LaunchDelegate interface not found.");
                    }
                }
                else
                {
                    LOGERR("Shell is not initialized.");
                }

                return mPlaybackClientPtr->PutResumePoint(appId, contentPartnerId, entityId, progress, completed, watchedOn);
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult XvpClientImplementation::GetAdIdentifier(const string &appId,
                                                               const Exchange::IXvpXifa::AdIdentifierScope &scope,
                                                               string &ifa, string &ifa_type, string &lmt)
        {
            LOGINFO("GetAdIdentifier for appId: %s, scope type: %s, scope id: %s", appId.c_str(), scope.type.c_str(), scope.id.c_str());
            if (mXifaClientPtr)
            {
                return mXifaClientPtr->GetAdIdentifier(appId, scope.type, scope.id, ifa, ifa_type, lmt);
            }
            return Core::ERROR_GENERAL;
        }

        Core::hresult XvpClientImplementation::ResetAdIdentifier(const string &appId,
                                                                 const Exchange::IXvpXifa::AdIdentifierScope &scope)
        {
            LOGINFO("ResetAdIdentifier for appId: %s, scope type: %s, scope id: %s", appId.c_str(), scope.type.c_str(), scope.id.c_str());
            if (mXifaClientPtr)
            {
                return mXifaClientPtr->ResetAdIdentifier(appId, scope.type, scope.id);
            }
            return Core::ERROR_GENERAL;
        }

    } // namespace Plugin
} // namespace WPEFramework
