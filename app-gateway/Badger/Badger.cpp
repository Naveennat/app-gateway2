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
#include "StringUtils.h"
#include "UtilsLogging.h"

#define API_VERSION_NUMBER_MAJOR BADGER_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR BADGER_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH BADGER_PATCH_VERSION

#define LAUNCHDELEGATE_CALLSIGN "org.rdk.LaunchDelegate"
#define OTT_SERVICES_CALLSIGN "org.rdk.OttServices"

namespace WPEFramework {

    namespace {
        static Plugin::Metadata<Plugin::Badger> metadata(
                // Version (Major, Minor, Patch)
                API_VERSION_NUMBER_MAJOR,
                API_VERSION_NUMBER_MINOR,
                API_VERSION_NUMBER_PATCH,
                // Preconditions
                {},
                // Terminations
                {},
                // Controls
                {});
    }

    namespace Plugin {
        SERVICE_REGISTRATION(Badger, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        Badger::Badger() : mService(nullptr), mConnectionId(0), mLaunchDelegate(nullptr) {
        }

        Badger::~Badger() {
        }

        /* virtual */ const string Badger::Initialize(PluginHost::IShell* service) {
            ASSERT(service != nullptr);
            LOGINFO("Initialize: PID=%u", getpid());

            mService = service;
            mService->AddRef();

            // Query LaunchDelegate interface
            mLaunchDelegate = mService->QueryInterfaceByCallsign<Exchange::ILaunchDelegate>(LAUNCHDELEGATE_CALLSIGN);
            if (mLaunchDelegate == nullptr) {
                LOGERR("LaunchDelegate not available");
            }

            mOttPermissions = mService->QueryInterfaceByCallsign<Exchange::IOttPermissions>(OTT_SERVICES_CALLSIGN);
            if (mOttPermissions == nullptr) {
                LOGERR("OttPermissions not available");
            }

            mDelegate = std::make_shared<DelegateHandler>();
            mDelegate->setShell(mService);

            return EMPTY_STRING;
        }

        /* virtual */ void Badger::Deinitialize(PluginHost::IShell* service) {
            ASSERT(service == mService);

            if (mLaunchDelegate) {
                mLaunchDelegate->Release();
                mLaunchDelegate = nullptr;
            }

            if (mOttPermissions) {
                mOttPermissions->Release();
                mOttPermissions = nullptr;
            }

            mDelegate->Cleanup();
            mDelegate.reset();

            mConnectionId = 0;
            mService->Release();
            mService = nullptr;
            LOGINFO("De-initialised");
        }

        string Badger::Information() const {
            string info = "Badger plugin";
            return info;
        }

        uint32_t Badger::ValidateCachedPermission(const std::string& appId, const std::string& requiredPermission) {
            if (mOttPermissions == nullptr) {
                LOGERR("OttPermissions interface not initialized");
                return Core::ERROR_UNAVAILABLE;
            }

            std::string permissions;
            uint32_t updatedCount = 0;

            if (mOttPermissions->GetPermissions(appId, permissions) != Core::ERROR_NONE) {
                LOGERR("GetPermissions failed for appId: %s", appId.c_str());
                return Core::ERROR_PRIVILIGED_REQUEST;
            }

            // Parse permissions JSON array
            Core::JSON::ArrayType<Core::JSON::String> permissionsArray;
            permissionsArray.FromString(permissions);

            bool isGranted = false;
            Core::JSON::ArrayType<Core::JSON::String>::Iterator index(permissionsArray.Elements());
            while (index.Next()) {
                if (index.Current().Value() == requiredPermission) {
                    isGranted = true;
                    break;
                }
            }

            if (!isGranted) {
                LOGERR("Permission denied: missing required permission '%s' "
                       "for %s",
                        requiredPermission.c_str(), appId.c_str());
                return Core::ERROR_PRIVILIGED_REQUEST;
            }

            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetHDCPStatus(const std::string& appId, std::string& hdcpJson) {
            hdcpJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.hdcp");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto hdcpDelegate = mDelegate->getHdcpProfileDelegate();
            if (!hdcpDelegate)
                return Core::ERROR_UNAVAILABLE;

            result = hdcpDelegate->GetHDCPStatus(hdcpJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("HdcpProfileDelegate::GetHDCPStatus failed");
                hdcpJson = R"({
            "connected": false,
            "hdcp_compliant": false,
            "hdcp_enabled": false,
            "supported_hdcp_version": "UNKNOWN",
            "receiver_hdcp_version": "UNKNOWN",
            "current_hdcp_version": "UNKNOWN"
        })";
                return Core::ERROR_NONE;
            }

            LOGINFO("HDCP JSON: %s", hdcpJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetHDRStatus(const std::string& appId, std::string& hdrJson) {
            hdrJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.hdr");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto displayDelegate = mDelegate->getDisplaySettingsDelegate();
            if (!displayDelegate)
                return Core::ERROR_UNAVAILABLE;

            result = displayDelegate->GetHDRSupport(hdrJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetHDRSupport failed in GetHDRStatus");
                hdrJson = R"({"settop_hdr_support":{},"tv_hdr_support":{}})";
                return Core::ERROR_NONE;
            }

            LOGINFO("HDR JSON: %s", hdrJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetAudioModeStatus(const std::string& appId, std::string& audioModeStatus) {
            audioModeStatus.clear();

            // If permission check is required later, uncomment:
            // uint32_t result = ValidateCachedPermission(appId, "API_UserData_audioMode");
            // if (result != Core::ERROR_NONE) {
            //     return result;
            // }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;

            auto displayDelegate = mDelegate->getDisplaySettingsDelegate();
            if (!displayDelegate)
                return Core::ERROR_UNAVAILABLE;

            uint32_t result = displayDelegate->GetAudioFormat(audioModeStatus);
            if (result != Core::ERROR_NONE) {
                LOGERR("DisplaySettingsDelegate::GetAudioFormat failed");
                return Core::ERROR_UNAVAILABLE;
            }

            LOGINFO("AudioMode JSON: %s", audioModeStatus.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetWebBrowserStatus(const std::string& appId, std::string& webBrowserStatusJson) {
            webBrowserStatusJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.webBrowser");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string platformConfigJson;
            result = systemDelegate->GetSystemPlatformConfiguration(platformConfigJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetSystemPlatformConfiguration failed in GetWebBrowserStatus");
                webBrowserStatusJson = R"({"browser_type":"UNKNOWN","version":"UNKNOWN","user_agent":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            WPEFramework::Core::JSON::VariantContainer platformConfig;
            platformConfig.FromString(platformConfigJson);

            WPEFramework::Core::JSON::VariantContainer browserJson;

            browserJson["browser_type"] = platformConfig["browser_type"].String().empty() ? "UNKNOWN" : platformConfig["browser_type"].String();

            browserJson["version"] = platformConfig["browser_version"].String().empty() ? "UNKNOWN" : platformConfig["browser_version"].String();

            browserJson["user_agent"] = platformConfig["browser_user_agent"].String().empty() ? "UNKNOWN" : platformConfig["browser_user_agent"].String();

            browserJson.ToString(webBrowserStatusJson);

            LOGINFO("WebBrowserStatus JSON: %s", webBrowserStatusJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetWiFiStatus(const std::string& appId, std::string& isWifiDeviceStatus) {
            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.isWifiDevice");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            // if (BadgerTypes::GetWiFiStatus(mWiFiLink, isWifiDeviceStatus) !=
            // Core::ERROR_NONE) {
            //   LOGERR("GetWiFiStatus failed");
            //   return Core::ERROR_UNAVAILABLE;
            // }
            isWifiDeviceStatus = "true";

            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetNativeDimensions(const std::string& appId, std::string& nativeDimensionsJson) {
            nativeDimensionsJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.nativeDimensions");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            uint32_t width = 3840;
            uint32_t height = 2160;

            WPEFramework::Core::JSON::VariantContainer json;
            WPEFramework::Core::JSON::VariantContainer dims;

            dims["0"] = Core::JSON::DecUInt32(width);
            dims["1"] = Core::JSON::DecUInt32(height);

            json["native_dimensions"] = dims;

            json.ToString(nativeDimensionsJson);

            LOGINFO("NativeDimensions JSON: %s", nativeDimensionsJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetVideoDimensions(const std::string& appId, std::string& videoDimensionsJson) {
            videoDimensionsJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.videoDimensions");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            uint32_t width = 1920;
            uint32_t height = 1080;

            WPEFramework::Core::JSON::VariantContainer dims;

            dims["0"] = Core::JSON::DecUInt32(width);
            dims["1"] = Core::JSON::DecUInt32(height);

            dims.ToString(videoDimensionsJson);

            LOGINFO("VideoDimensions JSON: %s", videoDimensionsJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetTimeZone(const std::string& appId, std::string& timeZoneJson) {
            timeZoneJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_timeZone");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            result = systemDelegate->GetTimeZone(timeZoneJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("SystemDelegate::GetTimeZone failed");
                timeZoneJson = R"({"time_zone":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            LOGINFO("TimeZone JSON: %s", timeZoneJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetDeviceType(const std::string& appId, std::string& deviceTypeJson) {
            deviceTypeJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.deviceType");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string platformConfigJson;
            result = systemDelegate->GetSystemPlatformConfiguration(platformConfigJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetSystemPlatformConfiguration failed in GetDeviceType");
                deviceTypeJson = R"({"device_type":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            WPEFramework::Core::JSON::VariantContainer platformConfig;
            platformConfig.FromString(platformConfigJson);

            std::string type = DelegateUtils::GetStringSafe(platformConfig, "device_type");

            WPEFramework::Core::JSON::VariantContainer resultJson;
            resultJson["device_type"] = type;

            resultJson.ToString(deviceTypeJson);

            LOGINFO("DeviceType JSON: %s", deviceTypeJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetDeviceModel(const std::string& appId, std::string& deviceModelJson) {
            deviceModelJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceCapabilities.model");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string platformConfigJson;
            result = systemDelegate->GetSystemPlatformConfiguration(platformConfigJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("GetSystemPlatformConfiguration failed in GetDeviceModel");

                deviceModelJson = R"({"device_model":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            WPEFramework::Core::JSON::VariantContainer platformConfig;
            platformConfig.FromString(platformConfigJson);

            std::string model = DelegateUtils::GetStringSafe(platformConfig, "model");

            WPEFramework::Core::JSON::VariantContainer resultJson;
            resultJson["device_model"] = model;

            resultJson.ToString(deviceModelJson);

            LOGINFO("DeviceModel JSON: %s", deviceModelJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::DeviceInfo(const std::string& appId, std::string& deviceInfoJson) {
            deviceInfoJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "API_UserData_deviceinfo");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            Core::JSON::VariantContainer response;
            Core::JSON::VariantContainer capabilities;

            std::string timeZoneJson;
            if (GetTimeZone(appId, timeZoneJson) != Core::ERROR_NONE) {
                LOGERR("GetTimeZone failed");
                return Core::ERROR_UNAVAILABLE;
            }
            Core::JSON::VariantContainer tzObj;
            tzObj.FromString(timeZoneJson);
            response["time_zone_offset"] = tzObj["time_zone"];

            std::string hdcpJson;
            GetHDCPStatus(appId, hdcpJson);
            Core::JSON::VariantContainer hdcpObj;
            hdcpObj.FromString(hdcpJson);
            capabilities["hdcp"] = hdcpObj;

            std::string hdrJson;
            GetHDRStatus(appId, hdrJson);
            Core::JSON::VariantContainer hdrObj;
            hdrObj.FromString(hdrJson);
            capabilities["hdr"] = hdrObj;

            std::string webBrowserJson;
            GetWebBrowserStatus(appId, webBrowserJson);
            Core::JSON::VariantContainer wbObj;
            wbObj.FromString(webBrowserJson);
            capabilities["web_browser"] = wbObj;

            std::string wifiJson;
            GetWiFiStatus(appId, wifiJson);
            Core::JSON::VariantContainer wifiObj;
            wifiObj.FromString(wifiJson);
            capabilities["is_wifi_device"] = wifiObj["is_wifi_device"];

            std::string nativeDimsJson;
            GetNativeDimensions(appId, nativeDimsJson);
            Core::JSON::VariantContainer ndObj;
            ndObj.FromString(nativeDimsJson);
            capabilities["native_dimensions"] = ndObj;

            std::string videoDimsJson;
            GetVideoDimensions(appId, videoDimsJson);
            Core::JSON::VariantContainer vdObj;
            vdObj.FromString(videoDimsJson);
            capabilities["video_dimensions"] = vdObj;

            std::string audioModesJson;
            GetAudioModeStatus(appId, audioModesJson);
            Core::JSON::VariantContainer audioObj;
            audioObj.FromString(audioModesJson);
            capabilities["audio_modes"] = audioObj;

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string platformJson;
            result = systemDelegate->GetSystemPlatformConfiguration(platformJson);
            if (result != Core::ERROR_NONE) {
                LOGWARN("Failed platform config, continuing with UNKNOWN fields");
                platformJson = R"({
            "account_id":"UNKNOWN","device_id":"UNKNOWN","model":"UNKNOWN","device_type":"UNKNOWN",
            "supports_true_sd":false,"browser_type":"UNKNOWN","browser_version":"UNKNOWN","browser_user_agent":"UNKNOWN"
        })";
            }

            Core::JSON::VariantContainer platform;
            platform.FromString(platformJson);

            response["device_id"] = platform["device_id"];
            response["account_id"] = platform["account_id"];
            capabilities["supports_true_sd"] = platform["supports_true_sd"];
            capabilities["receiver_platform"] = "RDK";
            capabilities["receiver_version"] = "1.0.0";

            response["zip_code"] = "UNKNOWN";
            response["receiver_id"] = "UNKNOWN";
            response["device_hash"] = "UNKNOWN";
            response["household_id"] = "UNKNOWN";
            WPEFramework::Core::JSON::VariantContainer privacyVC;
            privacyVC["trackingAllowed"] = Core::JSON::Boolean(false);
            response["privacy_settings"] = privacyVC;

            response["partner_id"] = "UNKNOWN";
            response["user_experience"] = "UNKNOWN";

            response["device_capabilities"] = capabilities;

            response.ToString(deviceInfoJson);
            return Core::ERROR_NONE;
        }

        uint32_t Badger::DeviceCapabilities(const std::string& appId, std::string& deviceCapabilitiesJson) {
            deviceCapabilitiesJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "API_DeviceCapabilities_deviceCapabilities");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            Core::JSON::VariantContainer capabilities;

            // ---- HDCP ----
            {
                std::string hdcpJson;
                GetHDCPStatus(appId, hdcpJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(hdcpJson);
                capabilities["hdcp"] = obj;
            }

            // ---- HDR ----
            {
                std::string hdrJson;
                GetHDRStatus(appId, hdrJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(hdrJson);
                capabilities["hdr"] = obj;
            }

            // ---- Audio Modes ----
            {
                std::string audioJson;
                GetAudioModeStatus(appId, audioJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(audioJson);
                capabilities["audio_modes"] = obj;
            }

            // ---- Web Browser ----
            {
                std::string wbJson;
                GetWebBrowserStatus(appId, wbJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(wbJson);
                capabilities["web_browser"] = obj;
            }

            // ---- WiFi ----
            {
                std::string wifiJson;
                GetWiFiStatus(appId, wifiJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(wifiJson);
                capabilities["is_wifi_device"] = obj["is_wifi_device"];
            }

            // ---- Native Dimensions ----
            {
                std::string ndJson;
                GetNativeDimensions(appId, ndJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(ndJson);
                capabilities["native_dimensions"] = obj;
            }

            // ---- Video Dimensions ----
            {
                std::string vdJson;
                GetVideoDimensions(appId, vdJson);
                Core::JSON::VariantContainer obj;
                obj.FromString(vdJson);
                capabilities["video_dimensions"] = obj;
            }

            // ---- Device Model / Type / supports_true_sd ----
            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string platformJson;
            result = systemDelegate->GetSystemPlatformConfiguration(platformJson);
            if (result != Core::ERROR_NONE) {
                LOGWARN("Platform config unavailable, defaulting values");
                platformJson = R"({"device_type":"UNKNOWN","model":"UNKNOWN","supports_true_sd":false})";
            }

            Core::JSON::VariantContainer platform;
            platform.FromString(platformJson);

            capabilities["device_type"] = DelegateUtils::GetStringSafe(platform, "device_type");
            capabilities["model"] = DelegateUtils::GetStringSafe(platform, "model");
            capabilities["supports_true_sd"] = platform["supports_true_sd"].Boolean();

            capabilities["receiver_platform"] = "RDK";
            capabilities["receiver_version"] = "1.0.0";

            capabilities.ToString(deviceCapabilitiesJson);

            LOGINFO("DeviceCapabilities JSON: %s", deviceCapabilitiesJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::NetworkConnectivity(const std::string& appId, std::string& connectivityJson) {
            connectivityJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "API_Network_networkConnectivity");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;
            auto networkDelegate = mDelegate->getNetworkDelegate();
            if (!networkDelegate)
                return Core::ERROR_UNAVAILABLE;

            result = networkDelegate->GetNetworkConnectivity(connectivityJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("NetworkDelegate::GetNetworkConnectivity failed");
                connectivityJson = R"({"status":"NO_ACTIVE_NETWORK_INTERFACE","networkInterface":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            LOGINFO("NetworkConnectivity JSON: %s", connectivityJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::Shutdown(const std::string& appId) {
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
            if (mLaunchDelegate->Close(launchDelegateCtx, "shutdown", result) != Core::ERROR_NONE) {
                LOGERR("LaunchDelegate Close failed");
                return Core::ERROR_UNAVAILABLE;
            }

            LOGINFO("Shutdown request processed for appId=%s", launchDelegateCtx.appId.c_str());

            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetDeviceId(const std::string& appId, std::string& deviceIdJson) {
            deviceIdJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_deviceId");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;

            auto authServiceDelegate = mDelegate->getAuthServiceDelegate();
            if (!authServiceDelegate)
                return Core::ERROR_UNAVAILABLE;

            result = authServiceDelegate->GetXDeviceId(deviceIdJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("AuthServiceDelegate::GetDeviceId failed in Badger::GetDeviceId");
                deviceIdJson = R"({"device_id":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            LOGINFO("DeviceId JSON: %s", deviceIdJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetDeviceName(const std::string& appId, std::string& deviceNameJson) {
            deviceNameJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_friendly_name");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;

            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            result = systemDelegate->GetFriendlyName(deviceNameJson);
            if (result != Core::ERROR_NONE) {
                LOGERR("SystemDelegate::GetFriendlyName failed in GetDeviceName");

                deviceNameJson = R"({"friendly_name":"Living Room"})";
                return Core::ERROR_NONE;
            }

            LOGINFO("DeviceName JSON: %s", deviceNameJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::DismissLoadingScreen(const std::string& appId) {
            uint32_t rc = ValidateCachedPermission(appId, "API_Navigation_dismissLoadingScreen");
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

            LOGINFO("DismissLoadingScreen processed for appId=%s", launchDelegateCtx.appId.c_str());

            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetLocalizationPostalCode(const std::string& appId, std::string& postalCodeJson) {
            postalCodeJson.clear();

            uint32_t result = ValidateCachedPermission(appId, "DATA_localization.postalCode");
            if (result != Core::ERROR_NONE) {
                return result;
            }

            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;

            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string countryCodeJson;
            result = systemDelegate->GetCountryCode(countryCodeJson);
            if (result != Core::ERROR_NONE) {
                LOGWARN("GetCountryCode failed, using fallback postal code");
                postalCodeJson = R"({"postal_code":"12345"})";
                return Core::ERROR_NONE;
            }

            WPEFramework::Core::JSON::VariantContainer countryVC;
            countryVC.FromString(countryCodeJson);

            std::string postal = DelegateUtils::GetStringSafe(countryVC, "country_code");

            WPEFramework::Core::JSON::VariantContainer resultJson;
            resultJson["postal_code"] = postal;

            resultJson.ToString(postalCodeJson);

            LOGINFO("Localization PostalCode JSON: %s", postalCodeJson.c_str());
            return Core::ERROR_NONE;
        }

        uint32_t Badger::ShowToaster(const std::string& appId, std::string& result) {
            uint32_t rc = ValidateCachedPermission(appId, "API_Notification_showToaster");
            if (rc != Core::ERROR_NONE) {
                return rc;
            }

            if (!mLaunchDelegate) {
                LOGERR("LaunchDelegate not available");
                return Core::ERROR_UNAVAILABLE;
            }

            Exchange::Context launchDelegateCtx;
            launchDelegateCtx.appId = appId;

            if (mLaunchDelegate->State(launchDelegateCtx, result) != Core::ERROR_NONE) {
                LOGERR("LaunchDelegate State failed");
                return Core::ERROR_GENERAL;
            }

            LOGINFO("ShowToaster processed for appId=%s", launchDelegateCtx.appId.c_str());

            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetDeviceUid(const std::string& appId, std::string& deviceUidJson) {
            deviceUidJson = R"({"device_uid":"1234567890"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::GetAccountUid(const std::string& appId, std::string& accountUidJson) {
            accountUidJson = R"({"account_uid":"0987654321"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::GetPayload(const std::string& appId, std::string& payloadJson) {
            payloadJson = R"({"payload":"sample_payload_data"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::OnLaunch(const std::string& appId, std::string& result) {
            result = R"({"status":"launch_handled"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::NavigateToCompanyPage(const std::string& appId, std::string& result) {
            result = R"({"status":"navigated_to_company_page"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::PromptEmail(const std::string& appId, std::string& result) {
            result = R"({"status":"email_prompted"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::ShowPinOverlay(const std::string& appId, std::string& result) {
            result = R"({"status":"pin_overlay_shown"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::Settings(const std::string& appId, std::string& result) {
            result.clear();

            // Follow the same call pattern as other getters:
            //  - obtain delegate from handler
            //  - call methods on delegate
            //  - assign to response/JSON
            if (!mDelegate)
                return Core::ERROR_UNAVAILABLE;

            auto systemDelegate = mDelegate->getSystemDelegate();
            if (!systemDelegate)
                return Core::ERROR_UNAVAILABLE;

            std::string legacyJson;
            std::string powerJson;

            if (systemDelegate->GetLegacyMiniGuide(legacyJson) != Core::ERROR_NONE) {
                LOGWARN("SystemDelegate::GetLegacyMiniGuide failed; using default {}");
                legacyJson = "{}";
            }
            if (systemDelegate->GetPowerSaveStatus(powerJson) != Core::ERROR_NONE) {
                LOGWARN("SystemDelegate::GetPowerSaveStatus failed; using default {}");
                powerJson = "{}";
            }

            Core::JSON::VariantContainer legacyObj;
            Core::JSON::VariantContainer powerObj;

            // Parse returned JSON; on parse failure keep empty object
            Core::OptionalType<Core::JSON::Error> parseErr;
            (void)parseErr;

            legacyObj.FromString(legacyJson);
            powerObj.FromString(powerJson);

            Core::JSON::VariantContainer response;
            response["legacyMiniGuide"] = legacyObj;
            response["power_save_status"] = powerObj;

            response.ToString(result);
            LOGINFO("Settings JSON: %s", result.c_str());
            return Core::ERROR_NONE;
        }
        uint32_t Badger::SubscribeToSettings(const std::string& appId, std::string& result) {
            result = R"({"status":"subscribed_to_settings"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::EntitlementsAccountLink(const std::string& appId, std::string& result) {
            result = R"({"status":"entitlements_account_linked"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::MediaEventAccountLink(const std::string& appId, std::string& result) {
            result = R"({"status":"media_event_account_linked"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::LaunchpadAccountLink(const std::string& appId, std::string& result) {
            result = R"({"status":"launchpad_account_linked"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::XIFA(const std::string& appId, std::string& result) {
            result = R"({"xifa_data":"sample_xifa_data"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::AppStoreId(const std::string& appId, std::string& result) {
            result = R"({"app_store_id":"com.example.app"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::LimitAdTracking(const std::string& appId, std::string& result) {
            result = R"({"limit_ad_tracking":false})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::DeviceAdAttributes(const std::string& appId, std::string& result) {
            result = R"({"ad_attributes":{"attribute1":"value1","attribute2":"value2"}})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::InitObject(const std::string& appId, std::string& result) {
            result = R"({"init_object":"sample_init_object"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::AppAuth(const std::string& appId, std::string& result) {
            result = R"({"app_auth_token":"sample_app_auth_token"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::OAuthBearerToken(const std::string& appId, std::string& result) {
            result = R"({"oauth_bearer_token":"sample_oauth_bearer_token"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::RefreshPlatformAuthToken(const std::string& appId, std::string& result) {
            result = R"({"status":"platform_auth_token_refreshed"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetXact(const std::string& appId, std::string& result) {
            result = R"({"xact_token":"sample_xact_token"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::NavigateToEntityPage(const std::string& appId, std::string& result) {
            result = R"({"status":"navigated_to_entity_page"})";
            return Core::ERROR_NONE;
        }
        uint32_t Badger::NavigateToFullScreenVideo(const std::string& appId, std::string& result) {
            result = R"({"status":"navigated_to_full_screen_video"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::LogMoneyBadgerLoaded(const std::string& appId, std::string& result) {
            result = R"({"status":"money_badger_loaded_logged"})";
            return Core::ERROR_NONE;
        }


        // Helper that encapsulates the logic previously in MetricsHandlerDelegate::HandleBadgerMetrics
        Core::hresult Badger::HandleMetricsProcessing(const std::string& appId,
                                                      const Core::JSON::VariantContainer& params)
        {
            auto metricsDelegate = (mDelegate ? mDelegate->getMetricsDelegate() : nullptr);
            if (!metricsDelegate) {
                LOGWARN("badger.metricsHandler: metrics delegate unavailable");
                return Core::ERROR_UNAVAILABLE;
            }

            using ParamKV = MetricsDelegate::ParamKV;

            // Flatten evt -> args
            std::vector<ParamKV> args;
            const Core::JSON::Variant evtVar = params["evt"];
            if (evtVar.IsSet() && !evtVar.IsNull() && evtVar.Content() == Core::JSON::Variant::type::OBJECT) {
                Core::JSON::VariantContainer evtObj = evtVar.Object();
                auto it = evtObj.Variants();
                while (it.Next()) {
                    ParamKV kv;
                    kv.name = it.Label();
                    kv.value = it.Current();
                    args.emplace_back(std::move(kv));
                }
            }

            auto ReadString = [](const Core::JSON::VariantContainer& obj, const char* key) -> std::string {
                const Core::JSON::Variant v = obj[key];
                if (v.IsSet() && !v.IsNull() && v.Content() == Core::JSON::Variant::type::STRING) {
                    return v.String();
                }
                return std::string();
            };

            auto ReadBool = [](const Core::JSON::VariantContainer& obj, const char* key, bool defVal) -> bool {
                const Core::JSON::Variant v = obj[key];
                if (v.IsSet() && !v.IsNull() && v.Content() == Core::JSON::Variant::type::BOOLEAN) {
                    return v.Boolean();
                }
                return defVal;
            };

            const Core::JSON::Variant segVar = params["segment"];
            if (segVar.IsSet() && !segVar.IsNull() && segVar.Content() == Core::JSON::Variant::type::STRING) {
                const std::string segment = segVar.String();
                if (segment == "LAUNCH_COMPLETED") {
                    return metricsDelegate->LaunchCompleted(args, appId);
                }
                Core::OptionalType<std::string> seg(segment);
                return metricsDelegate->MetricsHandler(seg, args, appId);
            }

            const Core::JSON::Variant evtTypeVar = params["eventType"];
            if (evtTypeVar.IsSet() && !evtTypeVar.IsNull() && evtTypeVar.Content() == Core::JSON::Variant::type::STRING) {
                const std::string eventType = evtTypeVar.String();
                const std::string eventTypeLower = StringUtils::toLower(eventType);

                if (eventTypeLower == "useraction") {
                    const std::string action = ReadString(params, "action");
                    return metricsDelegate->UserAction(action, args, appId);
                } else if (eventTypeLower == "appaction") {
                    const std::string action = ReadString(params, "action");
                    return metricsDelegate->AppAction(action, args, appId);
                } else if (eventTypeLower == "pageview") {
                    const std::string page = ReadString(params, "page");
                    return metricsDelegate->PageView(page, args, appId);
                } else if (eventTypeLower == "usererror" || eventTypeLower == "error") {
                    const std::string errMsg  = ReadString(params, "errMsg");
                    const std::string errCode = ReadString(params, "errCode");
                    const bool errVisible     = ReadBool(params, "errVisible", false);
                    if (eventTypeLower == "usererror") {
                        return metricsDelegate->UserError(errMsg, errVisible, errCode, args, appId);
                    } else {
                        return metricsDelegate->Error(errMsg, errVisible, errCode, args, appId);
                    }
                } else {
                    // Unknown eventType: route as generic app action with type=eventType
                    Core::OptionalType<std::string> seg(eventType);
                    return metricsDelegate->MetricsHandler(seg, args, appId);
                }
            }

            // Neither 'segment' nor 'eventType' provided, still send generic metrics to avoid drop
            Core::OptionalType<std::string> empty;
            return metricsDelegate->MetricsHandler(empty, args, appId);
        }

        uint32_t Badger::CompareAppSettings(const std::string& appId, std::string& result) {
            result = R"({"status":"tag: endOfLife - No longer supported"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::ResizeVideo(const std::string& appId, std::string& result) {
            result = R"({"status":"tag: endOfLife - No longer supported"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::XSCD(const std::string& appId, std::string& result) {
            result = R"({"xscd_token":"tag: endOfLife - No longer supported"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetOat(const std::string& appId, std::string& result) {
            result = R"({"oat_token":"tag: endOfLife - No longer supported"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::Deeplink(const std::string& appId, std::string& result) {
            result = R"({"status":"tag: endOfLife - No longer supported"})";
            return Core::ERROR_NONE;
        }

        uint32_t Badger::GetSystemInfo(const std::string& appId, std::string& result) {
            result = R"({"system_info":"tag: endOfLife - No longer supported"})";
            return Core::ERROR_NONE;
        }

        Core::hresult Badger::HandleAppGatewayRequest(const Exchange::Context& context, const std::string& method, const std::string& payload, std::string& result) {
            LOGTRACE("HandleAppGatewayRequest: method=%s, payload=%s, appId=%s", method.c_str(), payload.c_str(), context.appId.c_str());

            const std::string lower = StringUtils::toLower(method);

            static const std::unordered_map<std::string, std::function<Core::hresult(const std::string&, std::string&)>> simpleHandlers = {
                    {"info", [this](const std::string& appId, std::string& r) { return DeviceInfo(appId, r); }},
                    {"deviceCapabilities", [this](const std::string& appId, std::string& r) { return DeviceCapabilities(appId, r); }},
                    {"networkConnectivity", [this](const std::string& appId, std::string& r) { return NetworkConnectivity(appId, r); }},
                    {"getDeviceId", [this](const std::string& appId, std::string& r) { return GetDeviceId(appId, r); }},
                    {"getDeviceName", [this](const std::string& appId, std::string& r) { return GetDeviceName(appId, r); }},
                    {"localizationPostalCode", [this](const std::string& appId, std::string& r) { return GetLocalizationPostalCode(appId, r); }},
                    {"showToaster", [this](const std::string& appId, std::string& r) { return ShowToaster(appId, r); }},
                    {"getPayload", [this](const std::string& appId, std::string& r) { return GetPayload(appId, r); }},
                    {"onLaunch", [this](const std::string& appId, std::string& r) { return OnLaunch(appId, r); }},
                    {"navigateToCompanyPage", [this](const std::string& appId, std::string& r) { return NavigateToCompanyPage(appId, r); }},
                    {"promptEmail", [this](const std::string& appId, std::string& r) { return PromptEmail(appId, r); }},
                    {"showPinOverlay", [this](const std::string& appId, std::string& r) { return ShowPinOverlay(appId, r); }},
                    {"settings", [this](const std::string& appId, std::string& r) { return Settings(appId, r); }},
                    {"subscribeToSettings", [this](const std::string& appId, std::string& r) { return SubscribeToSettings(appId, r); }},
                    {"deviceUid", [this](const std::string& appId, std::string& r) { return GetDeviceUid(appId, r); }},
                    {"accountUid", [this](const std::string& appId, std::string& r) { return GetAccountUid(appId, r); }},
                    {"entitlementsAccountLink", [this](const std::string& appId, std::string& r) { return EntitlementsAccountLink(appId, r); }},
                    {"mediaEventAccountLink", [this](const std::string& appId, std::string& r) { return MediaEventAccountLink(appId, r); }},
                    {"launchpadAccountLink", [this](const std::string& appId, std::string& r) { return LaunchpadAccountLink(appId, r); }},
                    {"compareAppSettings", [this](const std::string& appId, std::string& r) { return CompareAppSettings(appId, r); }},
                    {"xifa", [this](const std::string& appId, std::string& r) { return XIFA(appId, r); }},
                    {"appStoreId", [this](const std::string& appId, std::string& r) { return AppStoreId(appId, r); }},
                    {"limitAdTracking", [this](const std::string& appId, std::string& r) { return LimitAdTracking(appId, r); }},
                    {"deviceAdAttributes", [this](const std::string& appId, std::string& r) { return DeviceAdAttributes(appId, r); }},
                    {"initObject", [this](const std::string& appId, std::string& r) { return InitObject(appId, r); }},
                    {"appAuth", [this](const std::string& appId, std::string& r) { return AppAuth(appId, r); }},
                    {"oauthBearerToken", [this](const std::string& appId, std::string& r) { return OAuthBearerToken(appId, r); }},
                    {"xscd", [this](const std::string& appId, std::string& r) { return XSCD(appId, r); }},
                    {"refreshPlatformAuthToken", [this](const std::string& appId, std::string& r) { return RefreshPlatformAuthToken(appId, r); }},
                    {"getOat", [this](const std::string& appId, std::string& r) { return GetOat(appId, r); }},
                    {"getXact", [this](const std::string& appId, std::string& r) { return GetXact(appId, r); }},
                    {"deeplink", [this](const std::string& appId, std::string& r) { return Deeplink(appId, r); }},
                    {"navigateToEntityPage", [this](const std::string& appId, std::string& r) { return NavigateToEntityPage(appId, r); }},
                    {"navigateToFullScreenVideo", [this](const std::string& appId, std::string& r) { return NavigateToFullScreenVideo(appId, r); }},
                    {"resizeVideo", [this](const std::string& appId, std::string& r) { return ResizeVideo(appId, r); }},
                    {"logMoneyBadgerLoaded", [this](const std::string& appId, std::string& r) { return LogMoneyBadgerLoaded(appId, r); }},
                    {"getSystemInfo", [this](const std::string& appId, std::string& r) { return GetSystemInfo(appId, r); }},
                    {"metricsHandler", [this](const std::string& appId, std::string& r) { return MetricsHandler(appId, r); }},
                    {"shutdown",
                            [this](const std::string& appId, std::string& r) {
                                (void) r;
                                return Shutdown(appId);
                            }},
                    {"dismissLoadingScreen",
                            [this](const std::string& appId, std::string& r) {
                                (void) r;
                                return DismissLoadingScreen(appId);
                            }},
            };

            // Run setter handler
            if (auto it = simpleHandlers.find(lower); it != simpleHandlers.end()) {
                std::string appId = context.appId;
                return it->second(appId, result);
            }

                        // badger.metricsHandler - accepts multiple shapes and returns true
            if (lower == "badger.metricshandler") {
                Core::JSON::VariantContainer params;
                Core::OptionalType<Core::JSON::Error> parseError;
                if (!params.FromString(payload, parseError)) {
                    LOGERR("badger.metricsHandler: invalid JSON params: %s", payload.c_str());
                    return Core::ERROR_BAD_REQUEST; // structural parse error -> let gateway send error
                }

                LOGDBG("badger.metricsHandler: received params: %s", payload.c_str());

                Core::hresult mhResult = HandleMetricsProcessing(context.appId, params);

                if (mhResult != Core::ERROR_NONE) {
                    LOGWARN("badger.metricsHandler: helper returned rc=%d (fire-and-forget semantics)", mhResult);
                }

                // Return boolean true to satisfy JSON-RPC result expectations
                result = "true";
                return Core::ERROR_NONE;
            }
            
            // TODO: uncomment below 2 lines in RDKE
            // ErrorUtils::NotSupported(result);
            // LOGERR("Unsupported method: %s", method.c_str());
            return Core::ERROR_UNKNOWN_KEY;
        }

    }  // namespace Plugin
}  // namespace WPEFramework