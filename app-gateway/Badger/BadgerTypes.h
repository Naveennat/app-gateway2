/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2024 RDK Management
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
#pragma once

#include "Module.h"
#include "UtilsJsonrpcDirectLink.h"
#include <core/Enumerate.h>
#include <core/JSON.h>
#include <core/core.h>

namespace WPEFramework {
namespace Plugin {

// -------------------- Empty Result --------------------
struct JBadgerEmptyResult : public Core::JSON::Container {
  JBadgerEmptyResult() : Core::JSON::Container() {}
};

// -------------------- Badger Request --------------------
struct JBadgerRequest : public Core::JSON::Container {
public:
  Core::JSON::String AppId;

  JBadgerRequest() : Core::JSON::Container() { Add(_T("appId"), &AppId); }

  string Get() const { return AppId.Value(); }
};


struct JBadgerHdrProfile : public Core::JSON::Container {
  JBadgerHdrProfile() {
    Add("supportsHDR", &supports_hdr);
    Add("standards", &standards);
  }

  Core::JSON::Boolean supports_hdr;
  Core::JSON::ArrayType<Core::JSON::String> standards;
};


struct JBadgerHDR : public Core::JSON::Container {
  JBadgerHDR() {
    Add("settop_hdr_support", &settop_hdr_support);
    Add("tv_hdr_support", &tv_hdr_support);
  }

  Core::JSON::ArrayType<Core::JSON::VariantContainer> settop_hdr_support;
  Core::JSON::ArrayType<Core::JSON::VariantContainer> tv_hdr_support;
};

// -------------------- Web Browser --------------------
struct JBadgerWebBrowser : public Core::JSON::Container {
  JBadgerWebBrowser() {
    Add("userAgent", &user_agent);
    Add("version", &version);
    Add("browserType", &browser_type);
  }
  Core::JSON::String user_agent;
  Core::JSON::String version;
  Core::JSON::String browser_type;
};

// -------------------- HDCP --------------------
struct JBadgerHDCP : public Core::JSON::Container {
  JBadgerHDCP() : connected(false), hdcp_compliant(false), hdcp_enabled(false) {
    Add("supportedHDCPVersion", &supported_hdcp_version);
    Add("receiverHDCPVersion", &receiver_hdcp_version);
    Add("currentHDCPVersion", &current_hdcp_version);
    Add("connected", &connected);
    Add("hdcpCompliant", &hdcp_compliant);
    Add("hdcpEnabled", &hdcp_enabled);
  }

  Core::JSON::String supported_hdcp_version;
  Core::JSON::String receiver_hdcp_version;
  Core::JSON::String current_hdcp_version;
  Core::JSON::Boolean connected;
  Core::JSON::Boolean hdcp_compliant;
  Core::JSON::Boolean hdcp_enabled;
};

// -------------------- Network Connectivity --------------------
enum class BadgerNetworkConnectivityStatus {
  NO_ACTIVE_NETWORK_INTERFACE,
  SUCCESS
};

struct NetworkConnectivityStatusEnum
    : public Core::JSON::EnumType<BadgerNetworkConnectivityStatus> {
  NetworkConnectivityStatusEnum()
      : Core::JSON::EnumType<BadgerNetworkConnectivityStatus>(
            BadgerNetworkConnectivityStatus::NO_ACTIVE_NETWORK_INTERFACE) {}

  NetworkConnectivityStatusEnum(BadgerNetworkConnectivityStatus v)
      : Core::JSON::EnumType<BadgerNetworkConnectivityStatus>(v) {}

  void SetValue(BadgerNetworkConnectivityStatus v) {
    Core::JSON::EnumType<BadgerNetworkConnectivityStatus>::operator=(v);
  }
};

struct JBadgerNetworkConnectivity : public Core::JSON::Container {
  JBadgerNetworkConnectivity() {
    Add("networkInterface", &network_interface);
    Add("status", &status);
  }
  Core::JSON::String network_interface;
  NetworkConnectivityStatusEnum status;
};

// -------------------- Audio Modes --------------------
struct JBadgerAudioModes : public Core::JSON::Container {
  JBadgerAudioModes() {
    Add("currentAudioMode", &current_audio_mode);
    Add("supportedAudioModes", &supported_audio_modes);
  }
  Core::JSON::String current_audio_mode;
  Core::JSON::ArrayType<Core::JSON::String> supported_audio_modes;
};

// -------------------- Device Capabilities --------------------
struct JBadgerDeviceCapabilities : public Core::JSON::Container {
  JBadgerDeviceCapabilities() {
    Add("deviceType", &device_type);
    Add("isWifiDevice", &is_wifi_device);
    Add("videoDimensions", &video_dimensions);
    Add("nativeDimensions", &native_dimensions);
    Add("model", &model);
    Add("receiverPlatform", &receiver_platform);
    Add("receiverVersion", &receiver_version);
    Add("hdr", &hdr);
    Add("hdcp", &hdcp);
    Add("webBrowser", &web_browser);
    Add("supportsTrueSD", &supports_true_sd);
    Add("deviceMakeModel", &device_make_model);
    Add("audioModes", &audio_modes);
  }

  Core::JSON::String device_type;
  Core::JSON::Boolean is_wifi_device;
  Core::JSON::ArrayType<Core::JSON::DecUInt32> video_dimensions;
  Core::JSON::ArrayType<Core::JSON::DecUInt32> native_dimensions;
  Core::JSON::String model;
  Core::JSON::String receiver_platform;
  Core::JSON::String receiver_version;
  JBadgerHDR hdr;
  JBadgerHDCP hdcp;
  JBadgerWebBrowser web_browser;
  Core::JSON::Boolean supports_true_sd;
  Core::JSON::String device_make_model;
  JBadgerAudioModes audio_modes;
};

// -------------------- Device Info --------------------
struct JBadgerDeviceInfo : public Core::JSON::Container {
  JBadgerDeviceInfo() {
    Add("zipcode", &zip_code);
    Add("timeZone", &time_zone);
    Add("timeZoneOffset", &time_zone_offset);
    Add("receiverId", &receiver_id);
    Add("deviceHash", &device_hash);
    Add("deviceId", &device_id);
    Add("accountId", &account_id);
    Add("deviceCapabilities", &device_capabilities);
    Add("householdId", &household_id);
    Add("privacySettings", &privacy_settings);
    Add("partnerId", &partner_id);
    Add("userExperience", &user_experience);
  }

  Core::JSON::String zip_code;
  Core::JSON::String time_zone;
  Core::JSON::String time_zone_offset;
  Core::JSON::String receiver_id;
  Core::JSON::String device_hash;
  Core::JSON::String device_id;
  Core::JSON::String account_id;
  JBadgerDeviceCapabilities device_capabilities;
  Core::JSON::String household_id;
  Core::JSON::VariantContainer privacy_settings;
  Core::JSON::String partner_id;
  Core::JSON::String user_experience;
};

struct JBadgerSystemPlatformConfig : public Core::JSON::Container {
  JBadgerSystemPlatformConfig() {
    Add("accountId", &account_id);
    Add("deviceId", &device_id);
    Add("model", &model);
    Add("deviceType", &device_type);
    Add("supportsTrueSD", &supports_true_sd);
    Add("browserType", &browser_type);
    Add("browserVersion", &browser_version);
    Add("browserUserAgent", &browser_user_agent);
  }

  Core::JSON::String account_id;
  Core::JSON::String device_id;
  Core::JSON::String model;
  Core::JSON::String device_type;
  Core::JSON::Boolean supports_true_sd;
  Core::JSON::String browser_type;
  Core::JSON::String browser_version;
  Core::JSON::String browser_user_agent;
};

class BadgerTypes {
private:
  BadgerTypes() = delete;
  ~BadgerTypes() = delete;
  BadgerTypes(const BadgerTypes &) = delete;
  BadgerTypes &operator=(const BadgerTypes &) = delete;

public:
  static uint32_t GetTimeZoneFromSystem(
      const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> &systemLink,
      Core::JSON::String &timeZone) {
    if (timeZone.IsSet() && !timeZone.Value().empty()) {
      LOGINFO("Using cached timeZone: %s", timeZone.Value().c_str());
      return Core::ERROR_NONE;
    }

    if (systemLink != nullptr) {
      JsonObject params;
      JsonObject response;

      uint32_t systemRet = systemLink->Invoke<JsonObject, JsonObject>(
          _T("getTimeZone"), params, response);

      if (systemRet == Core::ERROR_NONE && response.HasLabel("timeZone")) {
        timeZone = response["timeZone"].String();
        LOGINFO("Extracted timeZone: %s", timeZone.Value().c_str());
      }
    } else {
      LOGERR("systemLink is null in GetTimeZoneFromSystem()");
      return Core::ERROR_GENERAL;
    }

    return Core::ERROR_NONE;
  }

  static uint32_t GetFriendlyName(
      const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> &systemLink,
      Core::JSON::String &friendlyName) {
    // If a friendly name is already cached, reuse it
    if (!friendlyName.Value().empty()) {
      LOGINFO("[Badger] Using cached friendly name: %s",
              friendlyName.Value().c_str());
      return Core::ERROR_NONE;
    }

    // Validate input link
    if (systemLink == nullptr) {
      LOGERR("GetFriendlyName(): systemLink is null");
      return Core::ERROR_GENERAL;
    }

    JsonObject params;
    JsonObject response;

    // Invoke getFriendlyName method on the System plugin
    uint32_t rc = systemLink->Invoke<JsonObject, JsonObject>(
        _T("getFriendlyName"), params, response);

    if (rc != Core::ERROR_NONE) {
      LOGERR("getFriendlyName failed, rc=%u", rc);
      return rc;
    }

    // Check and extract the friendly name
    if (response.HasLabel("friendlyName")) {
      const string name = response["friendlyName"].String();

      if (!name.empty()) {
        friendlyName = name;
        LOGINFO("Friendly name fetched: %s", friendlyName.Value().c_str());
      } else {
        LOGERR("getFriendlyName returned empty 'friendlyName'");
        rc = Core::ERROR_GENERAL;
      }
    } else {
      LOGERR("getFriendlyName response missing 'friendlyName'");
      rc = Core::ERROR_GENERAL;
    }

    return rc;
  }

  static uint32_t
  GetAudioFormat(const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink>
                     &displaySettingsLink,
                 JBadgerAudioModes &audioModes) {
    if (audioModes.IsSet()) {
      LOGINFO("[Badger] Using cached AudioModes info");
      return Core::ERROR_NONE;
    } else {
      if (displaySettingsLink == nullptr) {
        LOGERR("displaySettingsLink is null in GetAudioFormat()");
        return Core::ERROR_GENERAL;
      }

      JsonObject params;
      JsonObject response;

      uint32_t rc = displaySettingsLink->Invoke<JsonObject, JsonObject>(
          _T("getAudioFormat"), params, response);

      if (rc == Core::ERROR_NONE) {
        if (response.HasLabel("currentAudioFormat")) {
          audioModes.current_audio_mode =
              response["currentAudioFormat"].String();
        }

        if (response.HasLabel("supportedAudioFormat")) {
          const JsonArray &arr = response["supportedAudioFormat"].Array();
          for (size_t i = 0; i < arr.Length(); i++) {
            Core::JSON::String fmt;
            fmt = arr[i].String();
            audioModes.supported_audio_modes.Add(fmt);
          }
        }

        LOGINFO("AudioFormat: current=%s, supportedCount=%d",
                audioModes.current_audio_mode.Value().c_str(),
                audioModes.supported_audio_modes.Length());
      } else {
        LOGERR("getAudioFormat failed, rc=%u", rc);
      }
      return rc;
    }
  }

  static uint32_t
  GetHDRSupport(const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink>
                    &displaySettingsLink,
                JBadgerHDR &hdrInfo) {
    if (hdrInfo.IsSet()) {
      LOGINFO("[Badger] Using cached HDR info");
      return Core::ERROR_NONE;
    }

    if (displaySettingsLink == nullptr) {
      LOGERR("displaySettingsLink is null in GetHDRSupport()");
      return Core::ERROR_GENERAL;
    }

    JsonObject params;
    JsonObject response;

    auto fillProfileArray =
        [](const JsonObject &resp,
           Core::JSON::ArrayType<Core::JSON::VariantContainer> &arr) {
          if (!resp.HasLabel("supportsHDR")) {
            return;
          }

          // Build JSON object for the profile
          JsonObject profile;
          profile["supportsHDR"] = resp["supportsHDR"].Boolean();
          if (resp.HasLabel("standards")) {
            profile["standards"] = resp["standards"];
          }

          // Add as VariantContainer
          auto &vc = arr.Add();
          Core::string jsonText;
          profile.ToString(jsonText);
          vc.FromString(jsonText);
        };

    // ---- Get Settop HDR Support ----
    response.Clear();
    uint32_t rc = displaySettingsLink->Invoke<JsonObject, JsonObject>(
        _T("getSettopHDRSupport"), params, response);
    if (rc == Core::ERROR_NONE) {
      fillProfileArray(response, hdrInfo.settop_hdr_support);
    } else {
      LOGERR("getSettopHDRSupport failed, rc=%u", rc);
    }

    // ---- Get TV HDR Support ----
    response.Clear();
    rc = displaySettingsLink->Invoke<JsonObject, JsonObject>(
        _T("getTvHDRSupport"), params, response);
    if (rc == Core::ERROR_NONE) {
      fillProfileArray(response, hdrInfo.tv_hdr_support);
    } else {
      LOGERR("getTvHDRSupport failed, rc=%u", rc);
    }

    return Core::ERROR_NONE;
  }

  static uint32_t GetHDCPStatus(
      const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> &hdcpLink,
      JBadgerHDCP &hdcpStatus) {
    if (hdcpStatus.IsSet()) {
      LOGINFO("[Badger] Using cached HdcpStatus");
      return Core::ERROR_NONE;
    } else {

      if (hdcpLink != nullptr) {
        JsonObject params;
        JsonObject response;

        uint32_t rc = hdcpLink->Invoke<JsonObject, JsonObject>(
            _T("getHDCPStatus"), params, response);

        if (rc == Core::ERROR_NONE && response.HasLabel("HDCPStatus")) {
          hdcpStatus.FromString(response["HDCPStatus"].String());

          LOGINFO("[New] HDCPStatus: connected=%s compliant=%s enabled=%s "
                  "supported=%s receiver=%s current=%s",
                  hdcpStatus.connected.Value() ? "true" : "false",
                  hdcpStatus.hdcp_compliant.Value() ? "true" : "false",
                  hdcpStatus.hdcp_enabled.Value() ? "true" : "false",
                  hdcpStatus.supported_hdcp_version.Value().c_str(),
                  hdcpStatus.receiver_hdcp_version.Value().c_str(),
                  hdcpStatus.current_hdcp_version.Value().c_str());
        } else {
          LOGERR("getHDCPStatus call failed or missing HDCPStatus, rc=%u", rc);
        }
      } else {
        LOGERR("hdcpLink is null in GetHDCPStatus()");
      }
      return Core::ERROR_NONE;
    }
  }

  static uint32_t GetNetworkConnectivity(
      const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink>
          &networkLink,
      JBadgerNetworkConnectivity &connectivity) {
    if (connectivity.IsSet()) {
      LOGINFO("[Badger] Using cached NetworkConnectivity info");
      return Core::ERROR_NONE;
    }

    connectivity.Clear();

    if (networkLink == nullptr) {
      LOGERR("networkLink is null in GetNetworkConnectivity()");
      connectivity.status.SetValue(
          Plugin::BadgerNetworkConnectivityStatus::NO_ACTIVE_NETWORK_INTERFACE);
      return Core::ERROR_GENERAL;
    }

    JsonObject params;
    JsonObject response;

    uint32_t rc = networkLink->Invoke<JsonObject, JsonObject>(
        _T("getDefaultInterface"), params, response);

    if (rc == Core::ERROR_NONE && response.HasLabel("interface")) {
      connectivity.network_interface = response["interface"].String();
      connectivity.status.SetValue(
          Plugin::BadgerNetworkConnectivityStatus::SUCCESS);

      LOGINFO("NetworkConnectivity: interface=%s, status=SUCCESS",
              connectivity.network_interface.Value().c_str());
    } else {
      LOGERR("getDefaultInterface failed or no interface found, rc=%u", rc);
      connectivity.status.SetValue(
          Plugin::BadgerNetworkConnectivityStatus::NO_ACTIVE_NETWORK_INTERFACE);
    }

    return rc;
  }

  static uint32_t GetSystemPlatformConfiguration(
      const std::shared_ptr<WPEFramework::Utils::JSONRPCDirectLink> &systemLink,
      JBadgerSystemPlatformConfig &platformConfig) {
    if (platformConfig.IsSet()) {
      LOGINFO("[Badger] Using cached PlatformConfig");
      return Core::ERROR_NONE;
    }

    if (systemLink == nullptr) {
      LOGERR("systemLink is null in GetSystemPlatformConfiguration()");
      return Core::ERROR_GENERAL;
    }

    JsonObject params;
    JsonObject response;

    params["query"] = "";

    uint32_t rc = systemLink->Invoke<JsonObject, JsonObject>(
        _T("getPlatformConfiguration"), params, response);

    if (rc == Core::ERROR_NONE) {
      // Flatten response if necessary
      if (response.HasLabel("AccountInfo")) {
        JsonObject accountInfo = response["AccountInfo"].Object();
        if (accountInfo.HasLabel("accountId")) {
          platformConfig.account_id = accountInfo["accountId"].String();
        }
        if (accountInfo.HasLabel("x1DeviceId")) {
          platformConfig.device_id = accountInfo["x1DeviceId"].String();
        }
      }

      if (response.HasLabel("DeviceInfo")) {
        JsonObject devInfo = response["DeviceInfo"].Object();
        if (devInfo.HasLabel("model")) {
          platformConfig.model = devInfo["model"].String();
        }
        if (devInfo.HasLabel("deviceType")) {
          platformConfig.device_type = devInfo["deviceType"].String();
        }
        if (devInfo.HasLabel("supportsTrueSD")) {
          platformConfig.supports_true_sd = devInfo["supportsTrueSD"].Boolean();
        }

        if (devInfo.HasLabel("webBrowser")) {
          JsonObject wb = devInfo["webBrowser"].Object();
          if (wb.HasLabel("browserType")) {
            platformConfig.browser_type = wb["browserType"].String();
          }
          if (wb.HasLabel("version")) {
            platformConfig.browser_version = wb["version"].String();
          }
          if (wb.HasLabel("userAgent")) {
            platformConfig.browser_user_agent = wb["userAgent"].String();
          }
        }
      }

      LOGINFO("Extracted PlatformConfig: accountId=%s, deviceId=%s, "
              "model=%s, deviceType=%s, supportsTrueSD=%s, "
              "browserType=%s, version=%s, userAgent=%s",
              platformConfig.account_id.Value().c_str(),
              platformConfig.device_id.Value().c_str(),
              platformConfig.model.Value().c_str(),
              platformConfig.device_type.Value().c_str(),
              platformConfig.supports_true_sd.Value() ? "true" : "false",
              platformConfig.browser_type.Value().c_str(),
              platformConfig.browser_version.Value().c_str(),
              platformConfig.browser_user_agent.Value().c_str());
    } else {
      LOGERR("getPlatformConfiguration call failed, error: %u", rc);
    }

    return rc;
  }
};

} // namespace Plugin

ENUM_CONVERSION_BEGIN(Plugin::BadgerNetworkConnectivityStatus){
    Plugin::BadgerNetworkConnectivityStatus::NO_ACTIVE_NETWORK_INTERFACE,
    _TXT("NO_ACTIVE_NETWORK_INTERFACE")},
    {Plugin::BadgerNetworkConnectivityStatus::SUCCESS, _TXT("SUCCESS")},
    ENUM_CONVERSION_END(Plugin::BadgerNetworkConnectivityStatus)

} // namespace WPEFramework
