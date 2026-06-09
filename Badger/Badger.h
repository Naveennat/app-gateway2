/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

#pragma once

#include "Module.h"
#include "UtilsJsonrpcDirectLink.h"
#include <interfaces/ILaunchDelegate.h>
#include <interfaces/IOttServices.h>
#include <interfaces/IAppGateway.h>
#include "Delegate/DelegateFactory.h"
#include <core/JSON.h>
#include <unordered_map>
#include <unordered_set>

#include "UtilsLogging.h"

namespace WPEFramework {
    namespace Plugin {

        /**
         * @brief Main Badger plugin class implementing OTT service gateway functionality
         * 
         * Provides a comprehensive gateway for OTT applications to access device capabilities,
         * advertising services, authentication, and various system features through a unified interface.
         */
        class Badger : public PluginHost::IPlugin, public Exchange::IAppGatewayRequestHandler {
          private:
            Badger(const Badger&) = delete;
            Badger& operator=(const Badger&) = delete;

          public:
            /**
             * @brief Default constructor for Badger plugin
             */
            Badger();

            /**
             * @brief Virtual destructor for proper cleanup
             */
            virtual ~Badger();

            // IPlugin
            /**
             * @brief Initialize the Badger plugin
             * @param shell The plugin shell interface for communication with Thunder framework
             * @return Error message if initialization failed, empty string on success
             */
            virtual const string Initialize(PluginHost::IShell* shell) override;

            /**
             * @brief Deinitialize the plugin and cleanup resources
             * @param service The plugin shell interface
             */
            virtual void Deinitialize(PluginHost::IShell* service) override;

            /**
             * @brief Get plugin information string
             * @return JSON string containing plugin metadata and version information
             */
            virtual string Information() const override;

          public:
            // IAppGatewayRequestHandler
            /**
             * @brief Handle application gateway requests from OTT applications
             * @param context The gateway context containing app information and permissions
             * @param method The method name being invoked
             * @param payload The JSON payload containing method parameters
             * @param result Output parameter for the JSON response
             * @return HRESULT indicating success or failure of the operation
             */
            virtual Core::hresult HandleAppGatewayRequest(const Exchange::GatewayContext& context /* @in */,
                    const string& method /* @in */,
                    const string& payload /* @in @opaque */,
                    string& result /* @out @opaque */) override;

            BEGIN_INTERFACE_MAP(Badger)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(Exchange::IAppGatewayRequestHandler)
            END_INTERFACE_MAP

          private:
            // -------- Internal Helpers --------
            /**
             * @brief Get application session identifier
             * @param appId The application identifier
             * @return Session ID string for the application
             */
            std::string GetAppSessionId(const string &appId);
            std::string GetDeviceSessionId(const Exchange::GatewayContext& context, const string &appId);
            std::string GetAppCatalogId(const string &appId);            
            /**
             * @brief Check if application has required permission
             * @param appId The application identifier
             * @param requiredPermission The permission string to validate
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult AuthorizeDataField(const std::string& appId, const char* requiredPermission);

            /**
             * @brief Clear permissions cache for a specific application
             * @param appId The application identifier to clear cache for
             */
            void ClearPermissionsCache(const std::string& appId);

            /**
             * @brief Clear all permissions from cache
             */
            void ClearAllPermissionsCache();

            /**
             * @brief Lazy loads and returns the LaunchDelegate interface
             * @return Pointer to ILaunchDelegate if successful, nullptr if failed
             * 
             * Initializes the LaunchDelegate interface on first call and caches it for subsequent calls.
             * Provides proper error handling and logging for interface acquisition failures.
             */
            Exchange::ILaunchDelegate* GetLaunchDelegate();

            /**
             * @brief Lazy loads and returns the OttServices interface
             * @return Pointer to IOttServices if successful, nullptr if failed
             * 
             * Initializes the OttServices interface on first call and caches it for subsequent calls.
             * Provides proper error handling and logging for interface acquisition failures.
             */
            Exchange::IOttServices* GetOttServices();

            /**
             * @brief Generate legacy unique identifier with application-specific magic
             * @param appId The application identifier
             * @param id The base identifier to transform
             * @param magic Application-specific magic string for uniqueness
             * @return Legacy UID string combining input parameters
             */
            std::string GetLegacyUID (const std::string& appId, const std::string& id, const std::string& magic);

            /**
             * @brief Filter null and empty values from JSON objects
             * @param input The JSON container to filter
             * @param output The filtered JSON container
             * 
             * Removes fields with null values, empty strings, and empty arrays/objects
             * from the JSON response to reduce payload size and improve API cleanliness.
             */
            void FilterNullAndEmptyValues(const Core::JSON::VariantContainer& input, Core::JSON::VariantContainer& output);

            /**
             * @brief Filter null and empty values from JSON string
             * @param jsonString The JSON string to filter (input and output)
             * 
             * Parses the JSON string, filters null/empty values, and updates the string.
             */
            void FilterNullAndEmptyValues(std::string& jsonString);

            // Uniform-signature wrappers for dispatch table (C++11-friendly)
            /**
            // -------- Device Capability / Info --------
            /**
             * @brief Get comprehensive device information
             * @param context Gateway context containing application information
             * @param deviceInfoJson Output JSON containing device details
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult DeviceInfo(const Exchange::GatewayContext& context, std::string& deviceInfoJson);

            /**
             * @brief Get device capabilities and supported features
             * @param context Gateway context containing application information
             * @param deviceCapabilitiesJson Output JSON containing device capabilities
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult DeviceCapabilities(const Exchange::GatewayContext& context, std::string& deviceCapabilitiesJson);

            /**
             * @brief Get network connectivity status
             * @param context Gateway context containing application information
             * @param connectivityJson Output JSON containing connectivity information
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult NetworkConnectivity(const Exchange::GatewayContext& context, std::string& connectivityJson);

            /**
             * @brief Get unique device identifier
             * @param context Gateway context containing application information
             * @param deviceIdJson Output JSON containing device ID
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetDeviceId(const Exchange::GatewayContext& context, std::string& deviceId);

            /**
             * @brief Get partner identifier associated with the device
             * @param context Gateway context containing application information
             * @param partnerId Output string containing partner ID
             * @param skipAuthorization Whether to skip default authorization check
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetPartnerId(const Exchange::GatewayContext& context, std::string& partnerId, const bool skipAuthorization = false);

            /**
             * @brief Get device friendly name
             * @param context Gateway context containing application information
             * @param deviceNameJson Output JSON containing device name
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetDeviceName(const Exchange::GatewayContext& context, std::string& deviceNameJson);

            /**
             * @brief Get current timezone information
             * @param context Gateway context containing application information
             * @param timeZoneJson Output JSON containing timezone data
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetTimeZone(const Exchange::GatewayContext& context, std::string& timeZoneJson);

            /**
             * @brief Get postal/zip code for localization
             * @param context Gateway context containing application information
             * @param zipCode Output string containing postal/zip code
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetZipCode(const WPEFramework::Exchange::GatewayContext& context, std::string& zipCode);

            /**
             * @brief Get HDCP (High-bandwidth Digital Content Protection) status
             * @param context Gateway context containing application information
             * @param hdcpJson Output JSON containing HDCP status information
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetHDCPStatus(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& hdcp);

            /**
             * @brief Get HDR (High Dynamic Range) status and capabilities
             * @param context Gateway context containing application information
             * @param hdrJson Output JSON containing HDR status information
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetHDRStatus(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& hdr);

            /**
             * @brief Get device type classification
             * @param context Gateway context containing application information
             * @param deviceType Output string containing device type value
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetDeviceType(const Exchange::GatewayContext& context, std::string& deviceType);

            /**
             * @brief Get audio mode configuration status
             * @param context Gateway context containing application information
             * @param audioModeStatus Output JSON containing audio mode information
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetAudioModeStatus(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& audioModeStatus);

            /**
             * @brief Get web browser availability and status
             * @param context Gateway context containing application information
             * @param webBrowserStatusJson Output JSON containing web browser status
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetWebBrowserStatus(const Exchange::GatewayContext& context, std::string& webBrowserStatusJson);

            /**
             * @brief Get WiFi connectivity status and information
             * @param context Gateway context containing application information
             * @param wifiJson Output JSON containing WiFi status details
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetWiFiStatus(const Exchange::GatewayContext& context, std::string& wifiJson);

            /**
             * @brief Get native display dimensions
             * @param context Gateway context containing application information
             * @param nativeDimensionsJson Output JSON containing native resolution
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetNativeDimensions(const Exchange::GatewayContext& context, Core::JSON::ArrayType<Core::JSON::Variant>& nativeDimensions);

            /**
             * @brief Get current video output dimensions
             * @param context Gateway context containing application information
             * @param videoDimensionsJson Output JSON containing video dimensions
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetVideoDimensions(const Exchange::GatewayContext& context, Core::JSON::ArrayType<Core::JSON::Variant>& videoDimensions);

            /**
             * @brief Get device model information
             * @param context Gateway context containing application information
             * @param deviceModelJson Output JSON containing device model details
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetDeviceModel(const Exchange::GatewayContext& context, std::string& deviceModelJson);

            /**
             * @brief Get localization postal code for regional content
             * @param context Gateway context containing application information
             * @param postalCode Output string containing postal code information
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetLocalizationPostalCode(const Exchange::GatewayContext& context, std::string& postalCode);

            // -------- UI / Interaction --------
            /**
             * @brief Display a toaster notification message
             * @param context Gateway context containing application information
             * @param result Output JSON containing operation result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult ShowToaster(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Get application launch payload data
             * @param context Gateway context containing application information
             * @param payloadJson Output JSON containing launch payload
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetPayload(const Exchange::GatewayContext& context, std::string& payloadJson);

            /**
             * @brief Handle application launch lifecycle event
             * @param context Gateway context containing application information
             * @param result Output JSON containing launch result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult OnLaunch(const Exchange::GatewayContext& context, const std::string& payload, std::string& result);

            /**
             * @brief Navigate to company information page
             * @param context Gateway context containing application information
             * @param result Output JSON containing navigation result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult NavigateToCompanyPage(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Navigate to entity-specific page
             * @param context Gateway context containing application information
             * @param result Output JSON containing navigation result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult NavigateToEntityPage(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Navigate to full-screen video player
             * @param context Gateway context containing application information
             * @param result Output JSON containing navigation result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult NavigateToFullScreenVideo(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Handle settings operations (get/set)
             * @param context Gateway context containing application information
             * @param payload Input JSON containing settings operation parameters
             * @param result Output JSON containing settings data or operation result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult Settings(const Exchange::GatewayContext& context, const std::string& payload, std::string& result);

            /**
             * @brief Subscribe to settings change notifications
             * @param context Gateway context containing application information
             * @param result Output JSON containing subscription result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult SubscribeToSettings(const Exchange::GatewayContext& context, std::string& result);

            // -------- Advertising --------
            /**
             * @brief Get XIFA (Cross-platform Identifier For Advertising)
             * @param context Gateway context containing application information
             * @param advertisingId Output string containing the advertising identifier
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult XIFA(const Exchange::GatewayContext& context, std::string& advertisingId);

            /**
             * @brief Get application store identifier
             * @param context Gateway context containing application information
             * @param result Output JSON containing app store ID
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult AppStoreId(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Check if ad tracking limitation is enabled
             * @param context Gateway context containing application information
             * @param limitAdTracking Output boolean indicating tracking limitation status
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult LimitAdTracking(const Exchange::GatewayContext& context, bool& limitAdTracking);

            /**
             * @brief Get device advertising attributes for targeting
             * @param context Gateway context containing application information
             * @param result Output JSON containing device ad attributes
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult DeviceAdAttributes(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Initialize advertising object with configuration options
             * @param context Gateway context containing application information
             * @param options Input JSON containing initialization options
             * @param result Output JSON containing initialization result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult InitObject(const Exchange::GatewayContext& context, const std::string& options, std::string& result);

            // -------- Auth / Tokens --------
            /**
             * @brief Perform application authentication
             * @param context Gateway context containing application information
             * @param result Output JSON containing authentication result and tokens
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult AppAuth(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Get OAuth bearer token for API access
             * @param context Gateway context containing application information
             * @param result Output JSON containing OAuth bearer token
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult OAuthBearerToken(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Refresh platform authentication token
             * @param context Gateway context containing application information
             * @param result Output JSON containing refreshed authentication token
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult RefreshPlatformAuthToken(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Get XACT (Cross-platform Access Control Token)
             * @param context Gateway context containing application information
             * @param result Output JSON containing XACT token
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetXact(const Exchange::GatewayContext& context, std::string& result);

            // -------- Account Linking --------
            /**
             * @brief Link account for media event tracking
             * @param context Gateway context containing application information
             * @param payload Input JSON containing account linking parameters
             * @param result Output JSON containing linking result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult MediaEventAccountLink(const Exchange::GatewayContext& context, const std::string& payload, std::string& result);

            /**
             * @brief Link account for entitlements management
             * @param context Gateway context containing application information
             * @param payload Input JSON containing entitlements linking parameters
             * @param result Output JSON containing linking result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult EntitlementsAccountLink(const Exchange::GatewayContext& context, const std::string& payload, std::string& result);

            /**
             * @brief Link account using legacy launchpad system
             * @param context Gateway context containing application information
             * @param payload Input JSON containing launchpad linking parameters
             * @param result Output JSON containing linking result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult LaunchpadAccountLink(const Exchange::GatewayContext& context, const std::string& payload, std::string& result);

            /**
             * @brief Log MoneyBadger loading event for analytics
             * @param context Gateway context containing application information
             * @param result Output JSON containing logging result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult LogMoneyBadgerLoaded(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Get STB (Set-Top Box) version information
             * @param stbVersion Output string containing STB version
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetSTBVersion(const Exchange::GatewayContext& context, std::string& stbVersion);

            /**
             * @brief Get device manufacturer and model information
             * @param makeAndModel Output string containing make and model
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetMakeAndModel(const Exchange::GatewayContext& context, std::string& makeAndModel);

            /**
             * @brief Get receiver software version information
             * @param receiverVersion Output string containing receiver version
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetReceiverVersion(const Exchange::GatewayContext& context, std::string& receiverVersion);

            /**
             * @brief Get account identifier for the requesting application
             * @param context Gateway context containing application information
             * @param accountId Output string containing account identifier
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetAccountId(const Exchange::GatewayContext& context, std::string& accountId);

            /**
             * @brief Get extended device identifier for cross-platform compatibility
             * @param context Gateway context containing application information
             * @param deviceIdJson Output JSON containing extended device ID
             * @param skipAuthorization Flag to skip authorization checks
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetXDeviceId(const Exchange::GatewayContext& context, std::string& deviceIdJson, const bool skipAuthorization = false);

            /**
             * @brief Get privacy settings configuration for the device
             * @param context Gateway context containing application information
             * @param privacySettings Output JSON containing privacy configuration
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult GetPrivacySettings(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& privacySettings);

            /**
             * @brief Handle metrics collection and reporting
             * @param context Gateway context containing application information
             * @param payload Input JSON containing metrics data
             * @param result Output JSON containing metrics handling result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult MetricsHandler(const Exchange::GatewayContext& context, const std::string& payload, std::string& result);

            // -------- Lifecycle --------
            /**
             * @brief Shutdown the application gracefully
             * @param context Gateway context containing application information
             * @param result Output JSON containing shutdown result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult Shutdown(const Exchange::GatewayContext& context, std::string& result);

            /**
             * @brief Dismiss the application loading screen
             * @param context Gateway context containing application information
             * @param result Output JSON containing dismiss result
             * @return Error code (0 on success, non-zero on failure)
             */
            Core::hresult DismissLoadingScreen(const Exchange::GatewayContext& context, std::string& result);

          private:
            /** @brief Thunder framework service shell interface */
            PluginHost::IShell* mService;

            /** @brief Connection ID for Thunder communication */
            uint32_t mConnectionId;

            /** @brief OTT services interface for access control */
            Exchange::IOttServices* mOttServices;

            /** @brief Delegate handler for service communication */
            std::shared_ptr<DelegateFactory> mDelegateFactory;

            /** @brief Cache for application permissions to avoid repeated OttServices calls */
            std::unordered_map<std::string, std::unordered_set<std::string>> mPermissionsCache;

            /** @brief Mutex to protect permissions cache access */
            mutable Core::CriticalSection mPermissionsCacheLock;
        };

    }  // namespace Plugin
}  // namespace WPEFramework
