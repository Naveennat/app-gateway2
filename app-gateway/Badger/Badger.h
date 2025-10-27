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

#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class Badger : public PluginHost::IPlugin, public PluginHost::JSONRPC {

private:
  Badger(const Badger &) = delete;
  Badger &operator=(const Badger &) = delete;

public:
  Badger();
  virtual ~Badger();
  virtual const string Initialize(PluginHost::IShell *shell) override;
  virtual void Deinitialize(PluginHost::IShell *service) override;
  virtual string Information() const override;

public:
  uint32_t BadgerDeviceInfo(const JBadgerRequest &request,
                            JBadgerDeviceInfo &response);
  uint32_t DeviceCapabilities(const JBadgerRequest &request,
                              JBadgerDeviceCapabilities &deviceCapabilities);
  uint32_t
  NetworkConnectivity(const JBadgerRequest &request,
                      JBadgerNetworkConnectivity &badgerNetworkConnectivity);
  uint32_t Shutdown(const JBadgerRequest &request);

  uint32_t GetDeviceId(const JBadgerRequest &request,
                       Core::JSON::String &deviceId);
  uint32_t GetDeviceName(const JBadgerRequest &request,
                         Core::JSON::String &deviceName);
  uint32_t DismissLoadingScreen(const JBadgerRequest &request);

  BEGIN_INTERFACE_MAP(Badger)
  INTERFACE_ENTRY(PluginHost::IPlugin)
  INTERFACE_ENTRY(PluginHost::IDispatcher)
  END_INTERFACE_MAP

private:
  void RegisterAll();
  void UnregisterAll();
  uint32_t ValidateCachedPermission(const std::string &appId,
                                    const std::string &requiredPermission);
  uint32_t GetTimeZone(std::string appId, Core::JSON::String &timeZone);
  uint32_t GetHDCPStatus(std::string appId, JBadgerHDCP &hdcpStatus);
  uint32_t GetHDRStatus(std::string appId, JBadgerHDR &hdrStatus);
  uint32_t GetDeviceType(std::string appId, Core::JSON::String &deviceType,
                         JBadgerSystemPlatformConfig &platformConfig);
  uint32_t GetAudioModeStatus(std::string appId,
                              JBadgerAudioModes &audioModeStatus);
  uint32_t GetWebBrowserStatus(std::string appId,
                               JBadgerWebBrowser &webBrowserStatus,
                               JBadgerSystemPlatformConfig &platformConfig);
  uint32_t GetWiFiStatus(std::string appId, Core::JSON::Boolean &isWifiDevice);
  uint32_t GetNativeDimensions(
      std::string appId,
      Core::JSON::ArrayType<Core::JSON::DecUInt32> &nativeDimensions);
  uint32_t GetVideoDimensions(
      std::string appId,
      Core::JSON::ArrayType<Core::JSON::DecUInt32> &videoDimensions);
  uint32_t GetDeviceModel(std::string appId, Core::JSON::String &model,
                          JBadgerSystemPlatformConfig &platformConfig);

private:
  PluginHost::IShell *mService;
  uint32_t mConnectionId;
  Exchange::ILaunchDelegate *mLaunchDelegate;
  Exchange::IOttServices *mOttPermissions;

public:
  // links to other plugins
  std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> mSystemLink;
  std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> mHdcpLink;
  std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> mdisplaySettingsLink;
  std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> mNetworkLink;
};
} // namespace Plugin
} // namespace WPEFramework
