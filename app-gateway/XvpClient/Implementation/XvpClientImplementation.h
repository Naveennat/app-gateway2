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
#include "UtilsLogging.h"
#include "DeviceInfo.h"
#include "XifaClient.h"
#include "PlaybackClient.h"
#include "VideoClient.h"
#include "SessionClient.h"
#include <interfaces/IXvpClient.h>
#include <interfaces/IConfiguration.h>
#include <memory>
#include <mutex>

namespace WPEFramework {
namespace Plugin {
    class XvpClientImplementation : public Exchange::IXvpSession, 
        public Exchange::IXvpPlayback, public Exchange::IXvpVideo,
        public Exchange::IXvpXifa, public Exchange::IConfiguration {
    private:
        XvpClientImplementation(const XvpClientImplementation&) = delete;
        XvpClientImplementation& operator=(const XvpClientImplementation&) = delete;

    public:
        XvpClientImplementation();
        ~XvpClientImplementation();

        BEGIN_INTERFACE_MAP(XvpClientImplementation)
        INTERFACE_ENTRY(Exchange::IXvpSession)
        INTERFACE_ENTRY(Exchange::IXvpXifa)
        INTERFACE_ENTRY(Exchange::IXvpVideo)
        INTERFACE_ENTRY(Exchange::IXvpPlayback)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);

        // IXvpSession interface
        Core::hresult ClearContentAccess(const string& appId ) override;
        Core::hresult SetContentAccess(const string& appId,
                                            const string& availabilities,
                                            const string& entitlements) override;

        // IXvpVideo interface
        Core::hresult SignIn(const string& appId, bool isSignedIn) override;

        // IXvpPlayback interface
        Core::hresult PutResumePoint(const string& appId,
            const string& entityId,
            const double progress,
            const bool completed,
            const string& watchedOn) override;

        // IXvpXifa interface
        Core::hresult GetAdIdentifier(const string& appId,
            const Exchange::IXvpXifa::AdIdentifierScope& scope,
            string& ifa, string& ifa_type, string& lmt) override;
        Core::hresult ResetAdIdentifier(const string& appId,
            const Exchange::IXvpXifa::AdIdentifierScope& scope) override;

    private:

        PluginHost::IShell* mShell;
        DeviceInfoPtr mDeviceInfoPtr;
        XifaClientPtr mXifaClientPtr;
        PlaybackClientPtr mPlaybackClientPtr;
        VideoClientPtr mVideoClientPtr;
        SessionClientPtr mSessionClientPtr;
    };
}
}
