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

#include "BadgerTypes.h"
#include "Module.h"
#include "UtilsJsonrpcDirectLink.h"
#include <interfaces/ILaunchDelegate.h>
#include <interfaces/IOttServices.h>
#include <interfaces/IAppGateway.h>
#include "Delegate/DelegateHandler.h"
#include <core/JSON.h>

#include "UtilsLogging.h"

namespace WPEFramework {
    namespace Plugin {

        class Badger : public PluginHost::IPlugin, Exchange::IAppGatewayRequestHandler {
          private:
            Badger(const Badger&) = delete;
            Badger& operator=(const Badger&) = delete;

          public:
            Badger();
            virtual ~Badger();
            virtual const string Initialize(PluginHost::IShell* shell) override;
            virtual void Deinitialize(PluginHost::IShell* service) override;
            virtual string Information() const override;

          public:
            virtual Core::hresult HandleAppGatewayRequest(const Exchange::GatewayContext& context /* @in */,
                    const string& method /* @in */,
                    const string& payload /* @in @opaque */,
                    string& result /*@out @opaque */) override;

            uint32_t DeviceInfo(const std::string appId, std::string& deviceInfoJson);
            uint32_t DeviceCapabilities(std::string appId, std::string& deviceCapabilitiesJson);
            uint32_t NetworkConnectivity(std::string appId, std::string& connectivityJson);
            uint32_t Shutdown(const std::string& appId);

            uint32_t GetDeviceId(std::string appId, std::string& deviceIdJson);
            uint32_t GetDeviceName(std::string appId, std::string& deviceNameJson);
            uint32_t DismissLoadingScreen(std::string appId);

            BEGIN_INTERFACE_MAP(Badger)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_ENTRY(Exchange::IAppGatewayRequestHandler)
            END_INTERFACE_MAP

          private:
            uint32_t ValidateCachedPermission(const std::string& appId, const std::string& requiredPermission);
            uint32_t GetTimeZone(std::string appId, std::string& timeZoneJson);
            uint32_t GetHDCPStatus(std::string appId, std::string& hdcpJson);
            uint32_t GetHDRStatus(std::string appId, std::string& hdrJson);
            uint32_t GetDeviceType(std::string appId, std::string& deviceTypeJson);
            uint32_t GetAudioModeStatus(std::string appId, JBadgerAudioModes& audioModeStatus);
            uint32_t GetWebBrowserStatus(std::string appId, std::string& webBrowserStatusJson);
            uint32_t GetWiFiStatus(std::string appId, Core::JSON::Boolean& isWifiDeviceStatus);
            uint32_t GetNativeDimensions(std::string appId, std::string& nativeDimensionsJson);
            uint32_t GetVideoDimensions(std::string appId, std::string& videoDimensionsJson);
            uint32_t GetDeviceModel(std::string appId, std::string& deviceModelJson);

            // Pending implementations for new methods
            uint32_t GetDeviceUid(const std::string& appId, std::string& deviceUidJson);
            uint32_t GetAccountUid(const std::string& appId, std::string& accountUidJson);
            uint32_t GetLocalizationPostalCode(const std::string& appId, std::string& postalCodeJson);
            uint32_t ShowToaster(const std::string& appId, std::string& result);
            uint32_t GetPayload(const std::string& appId, std::string& payloadJson);
            uint32_t OnLaunch(const std::string& appId, std::string& result);
            uint32_t NavigateToCompanyPage(const std::string& appId, std::string& result);
            uint32_t PromptEmail(const std::string& appId, std::string& result);
            uint32_t ShowPinOverlay(const std::string& appId, std::string& result);
            uint32_t Settings(const std::string& appId, std::string& result);
            uint32_t SubscribeToSettings(const std::string& appId, std::string& result);

          private:
            PluginHost::IShell* mService;
            uint32_t mConnectionId;
            Exchange::ILaunchDelegate* mLaunchDelegate;
            Exchange::IOttPermissions* mOttPermissions;
            std::shared_ptr<DelegateHandler> mDelegateHandler;
        };
    }  // namespace Plugin
}  // namespace WPEFramework
