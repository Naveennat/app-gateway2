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

#include "Badger.h"
#include "UtilsLogging.h"

#define API_VERSION_NUMBER_MAJOR BADGER_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR BADGER_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH BADGER_PATCH_VERSION

#define LAUNCHDELEGATE_CALLSIGN "org.rdk.LaunchDelegate"
#define SYSTEM_PLUGIN_CALLSIGN "org.rdk.System"
#define HDCP_PLUGIN_CALLSIGN "org.rdk.HdcpProfile"
#define DISPLAYSETTINGS_CALLSIGN "org.rdk.DisplaySettings"
#define NETWORK_CALLSIGN "org.rdk.Network"
#define OTT_SERVICES_CALLSIGN "org.rdk.OttServices"

namespace WPEFramework {

namespace {
static Plugin::Metadata<Plugin::Badger> metadata(
    // Version (Major, Minor, Patch)
    API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR,
    API_VERSION_NUMBER_PATCH,
    // Preconditions
    {},
    // Terminations
    {},
    // Controls
    {});
}

namespace Plugin {
SERVICE_REGISTRATION(Badger, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR,
                     API_VERSION_NUMBER_PATCH);

Badger::Badger()
    : PluginHost::JSONRPC(), mService(nullptr), mConnectionId(0),
      mLaunchDelegate(nullptr) {
  RegisterAll();
}

Badger::~Badger() { UnregisterAll(); }

/* virtual */ const string Badger::Initialize(PluginHost::IShell *service) {
  ASSERT(service != nullptr);
  LOGINFO("Initialize: PID=%u", getpid());

  mService = service;
  mService->AddRef();

  mSystemLink =
      Utils::GetThunderControllerClient(mService, SYSTEM_PLUGIN_CALLSIGN);
  mHdcpLink = Utils::GetThunderControllerClient(mService, HDCP_PLUGIN_CALLSIGN);
  mdisplaySettingsLink =
      Utils::GetThunderControllerClient(mService, DISPLAYSETTINGS_CALLSIGN);
  mNetworkLink = Utils::GetThunderControllerClient(mService, NETWORK_CALLSIGN);

  // Query LaunchDelegate interface
  mLaunchDelegate =
      mService->QueryInterfaceByCallsign<Exchange::ILaunchDelegate>(
          LAUNCHDELEGATE_CALLSIGN);
  if (mLaunchDelegate == nullptr) {
    LOGERR("LaunchDelegate not available");
  }

  mOttPermissions =
      mService->QueryInterfaceByCallsign<Exchange::IOttServices>(
          OTT_SERVICES_CALLSIGN);
  if (mOttPermissions == nullptr) {
    LOGERR("OttPermissions not available");
  }

  return EMPTY_STRING;
}

/* virtual */ void Badger::Deinitialize(PluginHost::IShell *service) {
  ASSERT(service == mService);

  if (mLaunchDelegate) {
    mLaunchDelegate->Release();
    mLaunchDelegate = nullptr;
  }

  if (mOttPermissions) {
    mOttPermissions->Release();
    mOttPermissions = nullptr;
  }

  mConnectionId = 0;
  mService->Release();
  mService = nullptr;
  LOGINFO("De-initialised");
}

string Badger::Information() const {
  string info = "Badger plugin";
  return info;
}
void Badger::RegisterAll() {
  Register<JBadgerRequest, JBadgerDeviceInfo>("info", &Badger::BadgerDeviceInfo,
                                              this);
  Register<JBadgerRequest, JBadgerDeviceCapabilities>(
      "deviceCapabilities", &Badger::DeviceCapabilities, this);
  Register<JBadgerRequest, JBadgerNetworkConnectivity>(
      "networkConnectivity", &Badger::NetworkConnectivity, this);
  Register<JBadgerRequest, void>("shutdown", &Badger::Shutdown, this);
  Register<JBadgerRequest, Core::JSON::String>("getDeviceId",
                                               &Badger::GetDeviceId, this);
  Register<JBadgerRequest, Core::JSON::String>("getDeviceName",
                                               &Badger::GetDeviceName, this);
  Register<JBadgerRequest, void>("dismissLoadingScreen",
                                 &Badger::DismissLoadingScreen, this);
}

void Badger::UnregisterAll() {
  Unregister("info");
  Unregister("deviceCapabilities");
  Unregister("networkConnectivity");
  Unregister("shutdown");
  Unregister("getDeviceId");
  Unregister("getDeviceName");
  Unregister("dismissLoadingScreen");
}

uint32_t
Badger::ValidateCachedPermission(const std::string &appId,
                                 const std::string &requiredPermission) {

  if (mOttPermissions == nullptr) {
    LOGERR("OttPermissions interface not initialized");
    return Core::ERROR_UNAVAILABLE;
  }

  std::string permissions;
  uint32_t updatedCount = 0;

  if (mOttPermissions->UpdatePermissionsCache(appId, updatedCount) !=
       Core::ERROR_NONE) {
    LOGERR("UpdatePermissionsCache failed for appId: %s", appId.c_str());
    return Core::ERROR_PRIVILIGED_REQUEST;
  }else {
    LOGINFO("Updated permissions cache for appId: %s, updatedCount=%u",
            appId.c_str(), updatedCount);
  }

  if (mOttPermissions->GetPermissions(appId, permissions) !=
       Core::ERROR_NONE) {
    LOGERR("GetPermissions failed for appId: %s", appId.c_str());
    return Core::ERROR_PRIVILIGED_REQUEST;
  }

  // Parse permissions JSON array
  Core::JSON::ArrayType<Core::JSON::String> permissionsArray;
  permissionsArray.FromString(permissions);

  bool isGranted = false;
  Core::JSON::ArrayType<Core::JSON::String>::Iterator index(
      permissionsArray.Elements());
  while (index.Next()) {
    if (index.Current().Value() == requiredPermission) {
      isGranted = true;
      break;
    }
  }

  if (!isGranted) {
    LOGERR("Permission denied: missing required permission '%s' for %s",
           requiredPermission.c_str(), appId.c_str());
    return Core::ERROR_PRIVILIGED_REQUEST;
  }

  return Core::ERROR_NONE;
}

uint32_t Badger::GetHDCPStatus(std::string appId, JBadgerHDCP &hdcpStatus) {
  uint32_t result = ValidateCachedPermission(appId, "API_UserData_hdcpstatus");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (BadgerTypes::GetHDCPStatus(mHdcpLink, hdcpStatus) != Core::ERROR_NONE) {
    LOGERR("GetHDCPStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  return Core::ERROR_NONE;
}

uint32_t Badger::GetHDRStatus(std::string appId, JBadgerHDR &hdrStatus) {
  uint32_t result = ValidateCachedPermission(appId, "API_UserData_hdrstatus");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (BadgerTypes::GetHDRSupport(mdisplaySettingsLink, hdrStatus) != Core::ERROR_NONE) {
    LOGERR("GetHDRSupport failed");
    return Core::ERROR_UNAVAILABLE;
  }

  return Core::ERROR_NONE;
}

uint32_t Badger::GetAudioModeStatus(std::string appId,
                                    JBadgerAudioModes &audioModeStatus) {
  uint32_t result = ValidateCachedPermission(appId, "API_UserData_audioMode");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (BadgerTypes::GetAudioFormat(mdisplaySettingsLink, audioModeStatus) !=
      Core::ERROR_NONE) {
    LOGERR("GetAudioModeStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  return Core::ERROR_NONE;
}

uint32_t Badger::GetWebBrowserStatus(std::string appId,
                                     JBadgerWebBrowser &webBrowserStatus,
                                     JBadgerSystemPlatformConfig &platformConfig) {
  uint32_t result = ValidateCachedPermission(appId, "API_UserData_webBrowser");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (platformConfig.IsSet()) {
    LOGINFO("Using cached WebBrowserStatus");
    webBrowserStatus.browser_type = platformConfig.browser_type;
    webBrowserStatus.version = platformConfig.browser_version;
    webBrowserStatus.user_agent = platformConfig.browser_user_agent;
    return Core::ERROR_NONE;
  } else {
    if (BadgerTypes::GetSystemPlatformConfiguration(
            mSystemLink, platformConfig) != Core::ERROR_NONE) {
      LOGERR("GetWebBrowserStatus failed");
      return Core::ERROR_UNAVAILABLE;
    }
    webBrowserStatus.browser_type = platformConfig.browser_type;
    webBrowserStatus.version = platformConfig.browser_version;
    webBrowserStatus.user_agent = platformConfig.browser_user_agent;
  }

  return Core::ERROR_NONE;
}

uint32_t Badger::GetWiFiStatus(std::string appId,
                               Core::JSON::Boolean &isWifiDeviceStatus) {
  uint32_t result =
      ValidateCachedPermission(appId, "DATA_deviceCapabilities.isWifiDevice");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  // if (BadgerTypes::GetWiFiStatus(mWiFiLink, isWifiDeviceStatus) !=
  // Core::ERROR_NONE) {
  //   LOGERR("GetWiFiStatus failed");
  //   return Core::ERROR_UNAVAILABLE;
  // }
  isWifiDeviceStatus = true;

  return Core::ERROR_NONE;
}

uint32_t Badger::GetNativeDimensions(
    std::string appId,
    Core::JSON::ArrayType<Core::JSON::DecUInt32> &nativeDimensions) {
  uint32_t result = ValidateCachedPermission(
      appId, "DATA_deviceCapabilities.nativeDimensions");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  // Simulate getting native dimensions
  Core::JSON::DecUInt32 width;
  width = 3840;
  Core::JSON::DecUInt32 height;
  height = 2160;
  nativeDimensions.Add(width);
  nativeDimensions.Add(height);

  return Core::ERROR_NONE;
}

uint32_t Badger::GetVideoDimensions(
    std::string appId,
    Core::JSON::ArrayType<Core::JSON::DecUInt32> &videoDimensions) {
  uint32_t result = ValidateCachedPermission(
      appId, "DATA_deviceCapabilities.videoDimensions");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  // Simulate getting video dimensions
  Core::JSON::DecUInt32 width;
  width = 1920;
  Core::JSON::DecUInt32 height;
  height = 1080;
  videoDimensions.Add(width);
  videoDimensions.Add(height);

  return Core::ERROR_NONE;
}

uint32_t Badger::GetTimeZone(std::string appId, Core::JSON::String &timeZone) {
  uint32_t result = ValidateCachedPermission(appId, "DATA_timeZone");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (BadgerTypes::GetTimeZoneFromSystem(mSystemLink, timeZone) !=
      Core::ERROR_NONE) {
    LOGERR("GetTimeZoneFromSystem failed");
    return Core::ERROR_UNAVAILABLE;
  }

  return Core::ERROR_NONE;
}

uint32_t Badger::GetDeviceType(std::string appId,
                               Core::JSON::String &deviceType,
                               JBadgerSystemPlatformConfig &platformConfig) {
  uint32_t result =
      ValidateCachedPermission(appId, "DATA_deviceCapabilities.deviceType");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (platformConfig.IsSet()) {
    LOGINFO("Using cached DeviceType");
    deviceType = platformConfig.device_type;
    return Core::ERROR_NONE;
  } else {
    if (BadgerTypes::GetSystemPlatformConfiguration(
            mSystemLink, platformConfig) != Core::ERROR_NONE) {
      LOGERR("GetDeviceType failed");
      return Core::ERROR_UNAVAILABLE;
    }
    deviceType = platformConfig.device_type;
  }
  return Core::ERROR_NONE;
}

uint32_t Badger::GetDeviceModel(std::string appId, Core::JSON::String &deviceModel,
                        JBadgerSystemPlatformConfig &platformConfig) {
  uint32_t result =
      ValidateCachedPermission(appId, "DATA_deviceCapabilities.model");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (platformConfig.IsSet()) {
    LOGINFO("Using cached DeviceModel");
    deviceModel = platformConfig.model;
    return Core::ERROR_NONE;
  } else {
    if (BadgerTypes::GetSystemPlatformConfiguration(
            mSystemLink, platformConfig) != Core::ERROR_NONE) {
      LOGERR("GetDeviceModel failed");
      return Core::ERROR_UNAVAILABLE;
    }
    deviceModel = platformConfig.model;
  }
  return Core::ERROR_NONE;
}

uint32_t Badger::BadgerDeviceInfo(const JBadgerRequest &request,
                                  JBadgerDeviceInfo &deviceInfo) {
  std::string appId = request.Get();
  JBadgerSystemPlatformConfig platformConfig;

  uint32_t result = ValidateCachedPermission(appId, "API_UserData_deviceinfo");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (GetTimeZone(appId, deviceInfo.time_zone_offset) != Core::ERROR_NONE) {
    LOGERR("GetTimeZone failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetHDCPStatus(appId, deviceInfo.device_capabilities.hdcp) != Core::ERROR_NONE) {
    LOGERR("GetHDCPStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetHDRStatus(appId, deviceInfo.device_capabilities.hdr) != Core::ERROR_NONE) {
    LOGERR("GetHDRStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetWebBrowserStatus(appId, deviceInfo.device_capabilities.web_browser,
                          platformConfig) != Core::ERROR_NONE) {
    LOGERR("GetWebBrowserStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetWiFiStatus(appId, deviceInfo.device_capabilities.is_wifi_device) !=
      Core::ERROR_NONE) {
    LOGERR("GetWiFiStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetNativeDimensions(appId,
                          deviceInfo.device_capabilities.native_dimensions) !=
      Core::ERROR_NONE) {
    LOGERR("GetNativeDimensions failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetVideoDimensions(appId,
                         deviceInfo.device_capabilities.video_dimensions) !=
      Core::ERROR_NONE) {
    LOGERR("GetVideoDimensions failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetAudioModeStatus(appId, deviceInfo.device_capabilities.audio_modes) !=
      Core::ERROR_NONE) {
    LOGERR("GetAudioModeStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetDeviceModel(appId, platformConfig.model, platformConfig) !=
      Core::ERROR_NONE) {
    LOGERR("GetDeviceModel failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetDeviceType(appId, platformConfig.device_type, platformConfig) !=
      Core::ERROR_NONE) {
    LOGERR("GetDeviceType failed");
    return Core::ERROR_UNAVAILABLE;
  }

  deviceInfo.device_id = platformConfig.device_id;
  deviceInfo.account_id = platformConfig.account_id;
  deviceInfo.device_capabilities.supports_true_sd =
      platformConfig.supports_true_sd;

  deviceInfo.zip_code = "UNKNOWN";
  deviceInfo.time_zone_offset = "UNKNOWN";
  deviceInfo.receiver_id = "UNKNOWN";
  deviceInfo.device_hash = "UNKNOWN";

  deviceInfo.device_capabilities.receiver_platform = "RDK";
  deviceInfo.device_capabilities.receiver_version = "1.0.0";

  deviceInfo.household_id = "UNKNOWN";
  deviceInfo.privacy_settings["trackingAllowed"] = false;
  deviceInfo.partner_id = "UNKNOWN";
  deviceInfo.user_experience = "UNKNOWN";

  return Core::ERROR_NONE;
}

uint32_t
Badger::DeviceCapabilities(const JBadgerRequest &request,
                           JBadgerDeviceCapabilities &deviceCapabilities) {
  std::string appId = request.Get();
  uint32_t result;
  JBadgerSystemPlatformConfig platformConfig;

  result = ValidateCachedPermission(
      appId, "API_DeviceCapabilities_deviceCapabilities");
  if (result != Core::ERROR_NONE) {
    return result;
  }

  if (GetHDCPStatus(appId, deviceCapabilities.hdcp) != Core::ERROR_NONE) {
    LOGERR("GetHDCPStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetHDRStatus(appId, deviceCapabilities.hdr) != Core::ERROR_NONE) {
    LOGERR("GetHDRStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetAudioModeStatus(appId, deviceCapabilities.audio_modes) !=
      Core::ERROR_NONE) {
    LOGERR("GetAudioModeStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetWebBrowserStatus(appId, deviceCapabilities.web_browser,
                          platformConfig) != Core::ERROR_NONE) {
    LOGERR("GetWebBrowserStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetWiFiStatus(appId, deviceCapabilities.is_wifi_device) !=
      Core::ERROR_NONE) {
    LOGERR("GetWiFiStatus failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetNativeDimensions(appId, deviceCapabilities.native_dimensions) !=
      Core::ERROR_NONE) {
    LOGERR("GetNativeDimensions failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetVideoDimensions(appId, deviceCapabilities.video_dimensions) !=
      Core::ERROR_NONE) {
    LOGERR("GetVideoDimensions failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetDeviceType(appId, deviceCapabilities.device_type, platformConfig) !=
      Core::ERROR_NONE) {
    LOGERR("GetDeviceType failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (GetDeviceModel(appId, deviceCapabilities.model, platformConfig) !=
      Core::ERROR_NONE) {
    LOGERR("GetDeviceModel failed");
    return Core::ERROR_UNAVAILABLE;
  }

  if (ValidateCachedPermission(appId,
                               "DATA_deviceCapabilities.deviceMakeModel") ==
      Core::ERROR_NONE) {
    deviceCapabilities.device_make_model = platformConfig.model;
  }

  deviceCapabilities.supports_true_sd = platformConfig.supports_true_sd;

  deviceCapabilities.receiver_platform = "RDK";
  deviceCapabilities.receiver_version = "1.0.0";

  return Core::ERROR_NONE;
}

uint32_t Badger::NetworkConnectivity(
    const JBadgerRequest &request,
    JBadgerNetworkConnectivity &badgerNetworkConnectivity) {

  uint32_t result = ValidateCachedPermission(request.Get(),
                                             "API_Network_networkConnectivity");
  if (result != Core::ERROR_NONE) {
    return result;
  } else {
    if (BadgerTypes::GetNetworkConnectivity(
            mNetworkLink, badgerNetworkConnectivity) != Core::ERROR_NONE) {
      LOGERR("GetNetworkConnectivity failed");
      return Core::ERROR_UNAVAILABLE;
    }
  }
}

uint32_t Badger::Shutdown(const JBadgerRequest &request) {
  std::string appId = request.Get();

  uint32_t rc = ValidateCachedPermission(appId, "API_Navigation_shutdown");
  if (rc != Core::ERROR_NONE) {
    return rc;
  }

  if (!mLaunchDelegate) {
    LOGERR("LaunchDelegate not available");
    return Core::ERROR_UNAVAILABLE;
  }

  Exchange::Context launchDelegateCtx;
  launchDelegateCtx.appId = appId;

  string result;
  if (mLaunchDelegate->Close(launchDelegateCtx, "shutdown", result) !=
      Core::ERROR_NONE) {
    LOGERR("LaunchDelegate Close failed");
    return Core::ERROR_UNAVAILABLE;
  }

  LOGINFO("Shutdown request processed for appId=%s",
          launchDelegateCtx.appId.c_str());

  return Core::ERROR_NONE;
}

uint32_t Badger::GetDeviceId(const JBadgerRequest &request,
                             Core::JSON::String &deviceId) {

  JBadgerSystemPlatformConfig platformConfig;
  uint32_t status =
      BadgerTypes::GetSystemPlatformConfiguration(mSystemLink, platformConfig);
  if (status != Core::ERROR_NONE) {
    LOGERR("BadgerTypes::GetSystemPlatformConfiguration failed with "
           "status=%u",
           status);
  }
  deviceId = platformConfig.device_id;

  return Core::ERROR_NONE;
}

uint32_t Badger::GetDeviceName(const JBadgerRequest &request,
                               Core::JSON::String &deviceName) {
  uint32_t result =
      ValidateCachedPermission(request.Get(), "DATA_friendly_name");
  if (result != Core::ERROR_NONE) {
    return result;
  } else {
    if (BadgerTypes::GetFriendlyName(mSystemLink, deviceName) !=
        Core::ERROR_NONE) {
      LOGERR("GetFriendlyName failed");
      return Core::ERROR_UNAVAILABLE;
    }
    return Core::ERROR_NONE;
  }
}

uint32_t Badger::DismissLoadingScreen(const JBadgerRequest &request) {
  std::string appId = request.Get();
  uint32_t rc = ValidateCachedPermission(appId,
                                         "API_Navigation_dismissLoadingScreen");
  if (rc != Core::ERROR_NONE) {
    return rc;
  }

  if (!mLaunchDelegate) {
    LOGERR("LaunchDelegate not available");
    return Core::ERROR_UNAVAILABLE;
  }

  Exchange::Context launchDelegateCtx;
  launchDelegateCtx.appId = appId;

  string result;
  if (mLaunchDelegate->Ready(launchDelegateCtx, result) != Core::ERROR_NONE) {
    LOGERR("LaunchDelegate Ready failed");
    return Core::ERROR_GENERAL;
  }

  LOGINFO("DismissLoadingScreen processed for appId=%s",
          launchDelegateCtx.appId.c_str());

  return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework