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
            // TODO: in RDKE this change Context to GatewayContext
            virtual Core::hresult HandleAppGatewayRequest(const Exchange::Context& context /* @in */,
                    const string& method /* @in */,
                    const string& payload /* @in @opaque */,
                    string& result /*@out @opaque */) override;

            BEGIN_INTERFACE_MAP(Badger)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            // INTERFACE_ENTRY(PluginHost::IDispatcher)
            END_INTERFACE_MAP

          private:
            uint32_t DeviceInfo(const std::string& appId, std::string& deviceInfoJson);
            uint32_t DeviceCapabilities(const std::string& appId, std::string& deviceCapabilitiesJson);
            uint32_t NetworkConnectivity(const std::string& appId, std::string& connectivityJson);
            uint32_t GetDeviceId(const std::string& appId, std::string& deviceIdJson);
            uint32_t GetDeviceName(const std::string& appId, std::string& deviceNameJson);
            uint32_t ValidateCachedPermission(const std::string& appId, const std::string& requiredPermission);
            uint32_t GetTimeZone(const std::string& appId, std::string& timeZoneJson);
            uint32_t GetHDCPStatus(const std::string& appId, std::string& hdcpJson);
            uint32_t GetHDRStatus(const std::string& appId, std::string& hdrJson);
            uint32_t GetDeviceType(const std::string& appId, std::string& deviceTypeJson);
            uint32_t GetAudioModeStatus(const std::string& appId, std::string& audioModeStatus);
            uint32_t GetWebBrowserStatus(const std::string& appId, std::string& webBrowserStatusJson);
            uint32_t GetWiFiStatus(const std::string& appId, std::string& isWifiDeviceStatus);
            uint32_t GetNativeDimensions(const std::string& appId, std::string& nativeDimensionsJson);
            uint32_t GetVideoDimensions(const std::string& appId, std::string& videoDimensionsJson);
            uint32_t GetDeviceModel(const std::string& appId, std::string& deviceModelJson);
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
            uint32_t EntitlementsAccountLink(const std::string& appId, std::string& result);
            uint32_t MediaEventAccountLink(const std::string& appId, std::string& result);
            uint32_t LaunchpadAccountLink(const std::string& appId, std::string& result);
            uint32_t CompareAppSettings(const std::string& appId, std::string& result);
            uint32_t XIFA(const std::string& appId, std::string& result);
            uint32_t AppStoreId(const std::string& appId, std::string& result);
            uint32_t LimitAdTracking(const std::string& appId, std::string& result);
            uint32_t DeviceAdAttributes(const std::string& appId, std::string& result);
            uint32_t InitObject(const std::string& appId, std::string& result);
            uint32_t AppAuth(const std::string& appId, std::string& result);
            uint32_t OAuthBearerToken(const std::string& appId, std::string& result);
            uint32_t XSCD(const std::string& appId, std::string& result);
            uint32_t RefreshPlatformAuthToken(const std::string& appId, std::string& result);
            uint32_t GetOat(const std::string& appId, std::string& result);
            uint32_t GetXact(const std::string& appId, std::string& result);
            uint32_t Deeplink(const std::string& appId, std::string& result);
            uint32_t NavigateToEntityPage(const std::string& appId, std::string& result);
            uint32_t NavigateToFullScreenVideo(const std::string& appId, std::string& result);
            uint32_t ResizeVideo(const std::string& appId, std::string& result);
            uint32_t LogMoneyBadgerLoaded(const std::string& appId, std::string& result);
            uint32_t GetSystemInfo(const std::string& appId, std::string& result);


            uint32_t Shutdown(const std::string& appId);
            uint32_t DismissLoadingScreen(const std::string& appId);

            // Helper that processes metrics payloads (moved from MetricsHandlerDelegate)
            Core::hresult HandleMetricsProcessing(const std::string& appId,
                                                  const Core::JSON::VariantContainer& params);

          private:
            PluginHost::IShell* mService;
            uint32_t mConnectionId;
            Exchange::ILaunchDelegate* mLaunchDelegate;
            Exchange::IOttPermissions* mOttPermissions;
            std::shared_ptr<DelegateHandler> mDelegate;
        };
    }  // namespace Plugin
}  // namespace WPEFramework
