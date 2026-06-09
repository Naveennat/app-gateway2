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

#include "Badger.h"
#include "StringUtils.h"
#include "UtilsCallsign.h"
#include "UtilsFirebolt.h"
#include "UtilsAppGatewayTelemetry.h"
#include "AppGatewayTelemetryMarkers.h"
#include "Delegate/DelegateUtils.h"
#include <unordered_map>
#include <unordered_set>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

#define API_VERSION_NUMBER_MAJOR BADGER_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR BADGER_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH BADGER_PATCH_VERSION

// Define plugin-specific telemetry client instance
AGW_DEFINE_TELEMETRY_CLIENT(AGW_PLUGIN_BADGER)

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

        /**
         * @brief Default constructor for Badger plugin
         * Initializes all member variables to their default states
         */
        Badger::Badger() : mService(nullptr), mConnectionId(0), mOttServices(nullptr), mDelegateFactory(nullptr) {
        }

        /**
         * @brief Destructor for Badger plugin
         * Performs cleanup of resources
         */
        Badger::~Badger() {
        }

        /**
         * @brief Initializes the Badger plugin
         * @param service Pointer to the plugin shell interface
         * @return Empty string on success, error message on failure
         * 
         * Sets up the plugin by initializing service references, querying for
         * LaunchDelegate and OttPermissions interfaces, and creating the delegate factory.
         */
        const string Badger::Initialize(PluginHost::IShell* service) {
            ASSERT(service != nullptr);
            LOGINFO("Initialize: PID=%u", getpid());
            
            mService = service;
            mService->AddRef();

            // Initialize telemetry client FIRST for reporting.
            AGW_TELEMETRY_INIT(mService);

            // Now Record bootstrap time
            AGW_RECORD_BOOTSTRAP_TIME();

            mDelegateFactory = std::make_shared<DelegateFactory>();
            mDelegateFactory->setShell(mService);

            return EMPTY_STRING;
        }

        /**
         * @brief Deinitializes the Badger plugin
         * @param service Pointer to the plugin shell interface
         * 
         * Cleans up resources by releasing interface references, cleaning up delegates,
         * permissions cache, and resetting connection state.
         */
        void Badger::Deinitialize(PluginHost::IShell* service) {
            ASSERT(service == mService);

            Exchange::IOttServices* ottServices = GetOttServices();
            if (ottServices) {
                ottServices->Release();
                mOttServices = nullptr;
            }

            // Deinitialize AppGateway telemetry client
            AGW_TELEMETRY_DEINIT();

            // Clear permissions cache during shutdown
            ClearAllPermissionsCache();

            if (mDelegateFactory) {
                mDelegateFactory->Cleanup();
                mDelegateFactory.reset();
            }

            mConnectionId = 0;
            if (mService) {
                mService->Release();
                mService = nullptr;
            }
            LOGINFO("De-initialised");
        }

        /**
         * @brief Provides information about the plugin
         * @return String describing the plugin
         */
        string Badger::Information() const {
            return "Badger plugin";
        }

        // ----------------- Helpers -----------------

        /**
         * @brief Retrieves application session identifier from LaunchDelegate
         * @param appId The application identifier
         * @return Session ID string for the application, or default value if failed
         * 
         * Queries the LaunchDelegate interface to obtain the session ID associated with the given application.
         * Returns "app_session_id.not.set" as fallback if interface is unavailable or operation fails.
         */
        std::string Badger::GetAppSessionId(const string& appId) {
            string sessionId = "app_session_id.not.set";

            if (mService != nullptr) {
                auto launchDelegate = mService->QueryInterfaceByCallsign<Exchange::IAppGatewayAuthenticator>(LAUNCH_DELEGATE_CALLSIGN);
                if (launchDelegate != nullptr) {
                    uint32_t err = launchDelegate->GetSessionId(appId, sessionId);
                    launchDelegate->Release();

                    if (err != Core::ERROR_NONE || sessionId.empty()) {
                        LOGERR("Failed to get app session ID for appId: %s, error: %u", appId.c_str(), err);
                    }
                } else {
                    LOGERR("LaunchDelegate interface not found.");
                }
            } else {
                LOGERR("Shell is not initialized.");
            }

            return sessionId;
        }

        std::string Badger::GetDeviceSessionId(const Exchange::GatewayContext& context, const string& appId) {            
            string deviceSessionId = "app_session_id.not.set";

            if (!mDelegateFactory) {
                LOGERR("DelegateFactory not initialized.");
            } else {
                auto lifecycle = mDelegateFactory->getDelegate<LifecycleDelegate>();
                if (!lifecycle) {
                    LOGERR("LifecycleDelegate not available.");
                    return deviceSessionId;
                }
                if (lifecycle->GetDeviceSessionId(context, deviceSessionId) != Core::ERROR_NONE) {
                    LOGERR("Failed to get device session ID for appId: %s", appId.c_str());
                    return deviceSessionId;
                }
            }
            return deviceSessionId;
        }

        /**
         * @brief Retrieves application catalog identifier from LaunchDelegate
         * @param appId The application identifier
         * @return Content partner ID string, or appId if not available
         * 
         * Queries the LaunchDelegate interface to obtain the content partner ID for the given application.
         * Returns appId as fallback if interface is unavailable or operation fails (matching Ripple behavior).
         */
        std::string Badger::GetAppCatalogId(const string& appId) {
            string contentPartnerId = appId; // Default to appId

            if (mService != nullptr) {
                auto launchDelegate = mService->QueryInterfaceByCallsign<Exchange::ILaunchDelegate>(LAUNCH_DELEGATE_CALLSIGN);
                if (launchDelegate != nullptr) {
                    string retrievedPartnerId;
                    uint32_t err = launchDelegate->GetContentPartnerId(appId, retrievedPartnerId);
                    launchDelegate->Release();

                    if (err == Core::ERROR_NONE && !retrievedPartnerId.empty()) {
                        contentPartnerId = std::move(retrievedPartnerId);
                    }
                }
            }

            LOGTRACE("GetAppCatalogId: appId=%s, contentPartnerId=%s", appId.c_str(), contentPartnerId.c_str());
            return contentPartnerId;
        }
        
        /**
         * @brief Checks if an application has the required authorization for a data field
         * @param appId The application identifier
         * @param requiredDataField The permission string to check for
         * @return Core::ERROR_NONE if permission granted, Core::ERROR_PRIVILIGED_REQUEST if denied,
         *         Core::ERROR_UNAVAILABLE if permission service unavailable
         * 
         * Uses a thread-safe cache to avoid repeated calls to OttServices for the same appId.
         * Cache is protected by mutex following the same pattern as delegate classes.
         */
        Core::hresult Badger::AuthorizeDataField(const std::string& appId, const char* requiredDataField) {
            // Create context for telemetry tracking
            Exchange::GatewayContext context{0, 0, appId};
            
            // Track API latency using scoped timer
            AGW_TRACK_API_CALL(tracker, context, "AuthorizeDataField");
            
            // First check cache with read lock
            {
                Core::SafeSyncType<Core::CriticalSection> lock(mPermissionsCacheLock);
                auto it = mPermissionsCache.find(appId);
                if (it != mPermissionsCache.end()) {
                    // Cache hit - check if permission exists
                    const auto& permissions = it->second;
                    if (permissions.find(requiredDataField) != permissions.end()) {
                        return Core::ERROR_NONE;
                    } else {
                        tracker.SetFailed(AGW_ERROR_PERMISSION_DENIED);
                        LOGWARN("Requested access '%s' denied for %s (cached)", requiredDataField, appId.c_str());
                        return Core::ERROR_PRIVILIGED_REQUEST;
                    }
                }
            }

            // Cache miss - need to fetch from OttServices
            Exchange::IOttServices* ottServices = GetOttServices();
            if (ottServices == nullptr) {
                LOGERR("OttServices interface not available");
                tracker.SetFailed(AGW_ERROR_INTERFACE_UNAVAILABLE);
                return Core::ERROR_UNAVAILABLE;
            }

            RPC::IStringIterator* permissionsIterator = nullptr;
            std::string permissions;
            if (ottServices->GetAppPermissions(appId, false, permissionsIterator) != Core::ERROR_NONE) {
                LOGERR("AuthorizeDataField failed for appId=%s", appId.c_str());
                tracker.SetFailed(AGW_ERROR_PERMISSION_DENIED);
                return Core::ERROR_PRIVILIGED_REQUEST;
            }

            if (permissionsIterator == nullptr) {
                LOGERR("AuthorizeDataField: GetAppPermissions returned success but no iterator for appId=%s", appId.c_str());
                tracker.SetFailed(AGW_ERROR_PERMISSION_DENIED);
                return Core::ERROR_PRIVILIGED_REQUEST;
            }

            // Collect all permissions first, then update cache with write lock
            std::unordered_set<std::string> collectedPermissions;
            while (permissionsIterator->Next(permissions)) {
                collectedPermissions.insert(permissions);
            }

            bool hasPermission = false;
            {
                Core::SafeSyncType<Core::CriticalSection> lock(mPermissionsCacheLock);
                auto& permissionSet = mPermissionsCache[appId];

                // Populate cache from locally collected permissions while holding the lock
                permissionSet.insert(collectedPermissions.begin(), collectedPermissions.end());

                // Check if the required permission exists
                hasPermission = (permissionSet.find(requiredDataField) != permissionSet.end());
            }
            if (permissionsIterator != nullptr) {
                permissionsIterator->Release();
                permissionsIterator = nullptr;
            }

            if (hasPermission) {
                return Core::ERROR_NONE;
            } else {
                tracker.SetFailed(AGW_ERROR_PERMISSION_DENIED);
                LOGWARN("Requested access '%s' denied for %s after refreshing permissions", requiredDataField, appId.c_str());
                return Core::ERROR_PRIVILIGED_REQUEST;
            }
        }

        /**
         * @brief Clears permissions cache for a specific application
         * @param appId The application identifier to clear cache for
         * 
         * Thread-safe removal of cached permissions for an application.
         * Useful when app permissions change or app is uninstalled.
         */
        void Badger::ClearPermissionsCache(const std::string& appId) {
            Core::SafeSyncType<Core::CriticalSection> lock(mPermissionsCacheLock);
            auto it = mPermissionsCache.find(appId);
            if (it != mPermissionsCache.end()) {
                mPermissionsCache.erase(it);
                LOGINFO("Cleared permissions cache for appId=%s", appId.c_str());
            }
        }

        /**
         * @brief Clears all permissions from cache
         * 
         * Thread-safe clearing of the entire permissions cache.
         * Useful during system restart or when permissions system is reloaded.
         */
        void Badger::ClearAllPermissionsCache() {
            Core::SafeSyncType<Core::CriticalSection> lock(mPermissionsCacheLock);
            size_t count = mPermissionsCache.size();
            mPermissionsCache.clear();
            LOGINFO("Cleared all permissions cache (%zu entries)", count);
        }

        /**
         * @brief Lazy loads and returns the OttPermissions interface
         * @return Pointer to IOttPermissions if successful, nullptr if failed
         * 
         * Initializes the OttServices interface on first call and caches it for subsequent calls.
         * Provides proper error handling and logging for interface acquisition failures.
         */
        Exchange::IOttServices* Badger::GetOttServices() {
            if (mOttServices == nullptr) {
                if (mService == nullptr) {
                    LOGERR("GetOttServices: Service not initialized");
                    return nullptr;
                }

                mOttServices = mService->QueryInterfaceByCallsign<Exchange::IOttServices>(OTT_SERVICES_CALLSIGN);
                if (mOttServices == nullptr) {
                    LOGERR("GetOttServices: Failed to acquire OttServices interface");
                } else {
                    LOGINFO("GetOttServices: Successfully acquired OttServices interface");
                }
            }

            return mOttServices;
        }

        /**
         * @brief Filter null and empty values from JSON objects
         * @param input The JSON container to filter
         * @param output The filtered JSON container
         * 
         * Removes fields with null values, empty strings, and empty arrays/objects
         * from the JSON response to reduce payload size and improve API cleanliness.
         * Preserves certain required fields even when empty for schema validation.
         */
        void Badger::FilterNullAndEmptyValues(const Core::JSON::VariantContainer& input, Core::JSON::VariantContainer& output) {
            output.Clear();
            
            // List of fields that should be preserved even when empty for schema validation
            // These fields must be present in JSON response even if empty
            static const std::unordered_set<std::string> requiredFields = {
                "currentAudioMode",
                "supportedAudioModes"
            };
            
            Core::JSON::VariantContainer::Iterator index = input.Variants();
            while (index.Next()) {
                const std::string& label = index.Label();
                const Core::JSON::Variant& value = index.Current();
                bool isRequiredField = (requiredFields.find(label) != requiredFields.end());
                
                // Skip null values for non-required fields only
                if (value.Content() == Core::JSON::Variant::type::EMPTY && !isRequiredField) {
                    continue;
                }
                
                // Skip empty strings (but allow "0", "false", etc.) for non-required fields only
                if (value.Content() == Core::JSON::Variant::type::STRING) {
                    const std::string strValue = value.String();
                    if (strValue.empty() && !isRequiredField) {
                        continue;
                    }
                    // Also skip strings that are just "null" for non-required fields
                    if (strValue == "null" && !isRequiredField) {
                        continue;
                    }
                }
                
                // Handle nested objects
                if (value.Content() == Core::JSON::Variant::type::OBJECT) {
                    const Core::JSON::VariantContainer& nestedInput = value.Object();
                    
                    Core::JSON::VariantContainer nestedOutput;
                    FilterNullAndEmptyValues(nestedInput, nestedOutput);
                    
                    // Always add nested objects that contain required fields, or have content after filtering
                    Core::JSON::VariantContainer::Iterator checkIterator = nestedOutput.Variants();
                    bool hasContent = checkIterator.Next();
                    
                    // For audioModes object, always preserve it if it contains currentAudioMode or supportedAudioModes
                    bool preserveAudioModes = (label == "audioModes");
                    
                    if (hasContent || preserveAudioModes || isRequiredField) {
                        output[label.c_str()] = nestedOutput;
                    }
                    continue;
                }
                
                // Handle arrays
                if (value.Content() == Core::JSON::Variant::type::ARRAY) {
                    // Access array directly instead of stringify/parse round-trip
                    const Core::JSON::ArrayType<Core::JSON::Variant>& arrayInput = value.Array();
                    Core::JSON::ArrayType<Core::JSON::Variant> arrayOutput;
                    
                    // Filter array elements
                    Core::JSON::ArrayType<Core::JSON::Variant>::ConstIterator arrayIndex = arrayInput.Elements();
                    while (arrayIndex.Next()) {
                        const Core::JSON::Variant& arrayValue = arrayIndex.Current();
                        
                        // Skip null/empty array elements
                        if (arrayValue.Content() == Core::JSON::Variant::type::EMPTY ||
                            (arrayValue.Content() == Core::JSON::Variant::type::STRING && 
                             (arrayValue.String().empty() || arrayValue.String() == "null"))) {
                            continue;
                        }
                        
                        // Handle nested objects in arrays
                        if (arrayValue.Content() == Core::JSON::Variant::type::OBJECT) {
                            const Core::JSON::VariantContainer& nestedArrayInput = arrayValue.Object();
                            Core::JSON::VariantContainer nestedArrayOutput;
                            FilterNullAndEmptyValues(nestedArrayInput, nestedArrayOutput);
                            
                            // Check if nested object has content
                            Core::JSON::VariantContainer::Iterator checkArrayIterator = nestedArrayOutput.Variants();
                            if (checkArrayIterator.Next()) {
                                arrayOutput.Add(nestedArrayOutput);
                            }
                        } else {
                            arrayOutput.Add(arrayValue);
                        }
                    }
                    
                    // Add array if it's not empty after filtering OR if it's a required field (preserve empty required arrays)
                    if (arrayOutput.Length() > 0 || isRequiredField) {
                        output[label.c_str()] = arrayOutput;
                    }
                    continue;
                }
                
                // Add all other non-empty values (numbers, booleans, etc.)
                output[label.c_str()] = value;
            }
        }

        /**
         * @brief Filter null and empty values from JSON string
         * @param jsonString The JSON string to filter (input and output)
         * 
         * Parses the JSON string, filters null/empty values, and updates the string.
         */
        void Badger::FilterNullAndEmptyValues(std::string& jsonString) {
            if (jsonString.empty()) {
                return;
            }
            
            try {
                Core::JSON::VariantContainer input;
                if (input.FromString(jsonString)) {
                    Core::JSON::VariantContainer output;
                    FilterNullAndEmptyValues(input, output);
                    
                    output.ToString(jsonString);
                } else {
                    LOGERR("FilterNullAndEmptyValues: Failed to parse JSON string, keeping original");
                    // Keep original string if parsing fails
                }
            } catch (const std::exception& e) {
                LOGERR("FilterNullAndEmptyValues: Failed to parse JSON: %s", e.what());
                // Keep original string if parsing fails
            } catch (...) {
                LOGERR("FilterNullAndEmptyValues: Unknown error occurred during JSON filtering");
                // Keep original string if parsing fails
            }
        }

        // ----------------- Device Capability Getters -----------------

        /**
         * @brief Retrieves HDCP status information for the device
         * @param appId The application identifier requesting the information
         * @param hdcpJson Output JSON string containing HDCP status details
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns HDCP connection status, compliance, enabled state, and version information.
         * Requires DATA_deviceCapabilities.hdcp permission.
         */
        Core::hresult Badger::GetHDCPStatus(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& hdcp) {
            hdcp.Clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.hdcp");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto hdcpLink = mDelegateFactory->getDelegate<HdcpProfileDelegate>();
            if (!hdcpLink)
                return Core::ERROR_UNAVAILABLE;

            std::string hdcpJson;
            hdcpLink->GetHDCPStatus(hdcpJson);
            hdcp.FromString(hdcpJson);

            // Filter out null/empty values
            Core::JSON::VariantContainer filteredHdcp;
            FilterNullAndEmptyValues(hdcp, filteredHdcp);
            hdcp = filteredHdcp;

            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves postal/zip code for device localization
         * @param context Gateway context containing application information
         * @param zipCode Output string containing postal/zip code
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns country-specific postal codes based on device location.
         * US devices return "66952", Canadian devices return "L6T 0C1".
         */
        Core::hresult Badger::GetZipCode(const Exchange::GatewayContext& context, std::string& zipCode) {
            zipCode.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_zipCode");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            std::string countryCode;
            system->GetCountryCode(context, countryCode);

            if (countryCode == "USA" || countryCode == "US") {
                zipCode = "66952";
            } else if (countryCode == "CAN" || countryCode == "CA") {
                zipCode = "L6T 0C1";
            } else {
                return Core::ERROR_UNAVAILABLE;
            }

            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves HDR (High Dynamic Range) support information
         * @param context Gateway context containing application information
         * @param hdr Output VariantContainer containing HDR support details
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns HDR support capabilities for both set-top box and TV.
         * Requires DATA_deviceCapabilities.hdr permission.
         */
        Core::hresult Badger::GetHDRStatus(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& hdr) {
            hdr.Clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.hdr");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            system->GetHDRSupport(context, hdr);

            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves current audio mode and supported audio modes
         * @param context Gateway context containing application information
         * @param audioModeStatus Output VariantContainer containing audio mode information
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the current audio format and list of supported audio modes.
         * Requires DATA_deviceCapabilities.audioModes permission.
         */
        Core::hresult Badger::GetAudioModeStatus(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& audioModeStatus) {
            audioModeStatus.Clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.audioModes");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto display = mDelegateFactory->getDelegate<DisplaySettingsDelegate>();
            if (!display)
                return Core::ERROR_UNAVAILABLE;

            if (display->GetAudioModeStatus(audioModeStatus) != Core::ERROR_NONE) {
                LOGERR("GetAudioModeStatus failed");
                return Core::ERROR_UNAVAILABLE;
            }

            // Filter out null/empty values
            Core::JSON::VariantContainer filteredAudioStatus;
            FilterNullAndEmptyValues(audioModeStatus, filteredAudioStatus);
            audioModeStatus = filteredAudioStatus;

            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves web browser information and capabilities
         * @param appId The application identifier requesting the information
         * @param webBrowserStatusJson Output JSON string containing browser details
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns browser type, version, and user agent information.
         * Requires DATA_deviceCapabilities.webBrowser permission.
         */
        Core::hresult Badger::GetWebBrowserStatus(const Exchange::GatewayContext& context, std::string& webBrowserStatusJson) {
            webBrowserStatusJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.webBrowser");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            std::string platformConfigJson;
            rc = system->GetSystemPlatformConfiguration(platformConfigJson);
            if (rc != Core::ERROR_NONE) {
                LOGERR("GetWebBrowserStatus: GetSystemPlatformConfiguration failed with error code %d, using default value", rc);
                webBrowserStatusJson = R"({"browser_type":"UNKNOWN","browser_version":"UNKNOWN","browser_user_agent":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }

            Core::JSON::VariantContainer platformConfig;
            platformConfig.FromString(platformConfigJson);
            Core::JSON::VariantContainer browserJson;

            browserJson["browser_type"] = platformConfig["browser_type"].String().empty() ? "UNKNOWN" : platformConfig["browser_type"].String();
            browserJson["browser_version"] = platformConfig["browser_version"].String().empty() ? "UNKNOWN" : platformConfig["browser_version"].String();
            browserJson["browser_user_agent"] = platformConfig["browser_user_agent"].String().empty() ? "UNKNOWN" : platformConfig["browser_user_agent"].String();

            // Filter out null/empty values before converting to string
            Core::JSON::VariantContainer filteredJson;
            FilterNullAndEmptyValues(browserJson, filteredJson);
            filteredJson.ToString(webBrowserStatusJson);
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves WiFi device capability status
         * @param appId The application identifier requesting the information
         * @param wifiJson Output JSON string indicating if device supports WiFi
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns whether the device has WiFi capabilities.
         * Requires DATA_deviceCapabilities.isWifiDevice permission.
         */
        Core::hresult Badger::GetWiFiStatus(const Exchange::GatewayContext& context, std::string& wifiJson) {
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.isWifiDevice");
            if (rc != Core::ERROR_NONE)
                return rc;
            wifiJson = R"({"is_wifi_device":true})";
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the native display dimensions of the device
         * @param appId The application identifier requesting the information
         * @param nativeDimensionsJson Output JSON string containing native resolution
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the device's native display resolution (typically 4K: 3840x2160).
         * Requires DATA_deviceCapabilities.nativeDimensions permission.
         */
        Core::hresult Badger::GetNativeDimensions(const Exchange::GatewayContext& context, Core::JSON::ArrayType<Core::JSON::Variant>& nativeDimensions) {
            nativeDimensions.Clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.nativeDimensions");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            std::string nativeDimensionsJson;
            rc = system->GetScreenResolution(context, nativeDimensionsJson);
            if (rc != Core::ERROR_NONE) {
                LOGERR("GetScreenResolution failed");
                return Core::ERROR_UNAVAILABLE;
            }

            nativeDimensions.FromString(nativeDimensionsJson);
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the video output dimensions
         * @param appId The application identifier requesting the information
         * @param videoDimensionsJson Output JSON string containing video resolution
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the video output resolution (typically HD: 1920x1080).
         * Requires DATA_deviceCapabilities.videoDimensions permission.
         */
        Core::hresult Badger::GetVideoDimensions(const Exchange::GatewayContext& context, Core::JSON::ArrayType<Core::JSON::Variant>& videoDimensions) {
            videoDimensions.Clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.videoDimensions");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            std::string videoResolutionJson;
            rc = system->GetVideoResolution(context, videoResolutionJson);
            if (rc != Core::ERROR_NONE) {
                LOGERR("GetVideoResolution failed");
                return Core::ERROR_UNAVAILABLE;
            }

            videoDimensions.FromString(videoResolutionJson);
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the device's time zone setting
         * @param appId The application identifier requesting the information
         * @param timeZoneJson Output JSON string containing time zone information
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the current time zone configuration of the device.
         * Requires DATA_timeZone permission.
         */
        Core::hresult Badger::GetTimeZone(const Exchange::GatewayContext& context, std::string& timeZoneJson) {
            timeZoneJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_timeZone");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            if (system->GetTimeZone(context, timeZoneJson) != Core::ERROR_NONE) {
                LOGERR("GetTimeZone failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the partner identifier from the authentication service
         * @param context Gateway context containing application information
         * @param partnerId Output string to store the partner identifier
         * @param skipAuthorization Whether to skip default authorization check
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Fetches the partner ID from the AuthService. This identifier
         * is used to identify the partner associated with the device.
         * Requires DATA_partnerId permission when skipAuthorization is false (default).
         * When skipAuthorization is true, the caller is responsible for performing
         * appropriate permission checks before calling this function.
         */
        Core::hresult Badger::GetPartnerId(const Exchange::GatewayContext& context, std::string& partnerId, const bool skipAuthorization) {
            partnerId.clear();
            
            if (!skipAuthorization) {
                Core::hresult rc = AuthorizeDataField(context.appId, "DATA_partnerId");
                if (rc != Core::ERROR_NONE)
                    return rc;
            }

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto authService = mDelegateFactory->getDelegate<AuthServiceDelegate>();
            if (!authService)
                return Core::ERROR_UNAVAILABLE;

            if (authService->GetPartnerId(partnerId) != Core::ERROR_NONE) {
                LOGERR("GetPartnerId failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves STB (Set-Top Box) software version
         * @param stbVersion Output string containing STB version information
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Delegates to SystemDelegate for STB version retrieval from system configuration.
         */
        Core::hresult Badger::GetSTBVersion(const Exchange::GatewayContext& context, std::string& stbVersion) {
            stbVersion.clear();

            Core::hresult rc1 = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.deviceVersion");
            Core::hresult rc2 = AuthorizeDataField(context.appId, "DATA_default_deviceCapabilities");
            if (rc1 != Core::ERROR_NONE && rc2 != Core::ERROR_NONE)
                return Core::ERROR_PRIVILIGED_REQUEST;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            if (system->GetSTBVersion(context, stbVersion) != Core::ERROR_NONE) {
                LOGERR("GetSTBVersion failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves device manufacturer and model information
         * @param makeAndModel Output string containing combined make and model
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Delegates to SystemDelegate for device make and model from system information.
         */
        Core::hresult Badger::GetMakeAndModel(const Exchange::GatewayContext& context, std::string& makeAndModel) {
            makeAndModel.clear();

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            if (system->GetMakeAndModel(context, makeAndModel) != Core::ERROR_NONE) {
                LOGERR("GetMakeAndModel failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves receiver software version information
         * @param receiverVersion Output string containing receiver version
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Delegates to SystemDelegate for receiver version from system configuration.
         */
        Core::hresult Badger::GetReceiverVersion(const Exchange::GatewayContext& context, std::string& receiverVersion) {
            receiverVersion.clear();

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            if (system->GetReceiverVersion(context, receiverVersion) != Core::ERROR_NONE) {
                LOGERR("GetReceiverVersion failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the type of device (STB, TV, etc.)
         * @param context Gateway context containing application information
         * @param deviceType Output string containing device type value
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the device type from FbEntos via SystemDelegate.
         * Requires DATA_deviceCapabilities.deviceType permission.
         */
        Core::hresult Badger::GetDeviceType(const Exchange::GatewayContext& context, std::string& deviceType) {
            deviceType.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.deviceType");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            rc = system->GetDeviceType(context, deviceType);
            if (rc != Core::ERROR_NONE) {
                LOGERR("GetDeviceType: SystemDelegate::GetDeviceType failed with error code %d", rc);
                return rc;
            }
            if (deviceType.empty()) {
                LOGERR("GetDeviceType: SystemDelegate::GetDeviceType returned empty deviceType");
                return Core::ERROR_GENERAL;
            }

            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the device model information
         * @param appId The application identifier requesting the information
         * @param deviceModelJson Output JSON string containing device model
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the device model from platform configuration.
         * Requires DATA_deviceCapabilities.model permission.
         */
        Core::hresult Badger::GetDeviceModel(const Exchange::GatewayContext& context, std::string& deviceModelJson) {
            deviceModelJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceCapabilities.model");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            std::string platformConfigJson;
            rc = system->GetSystemPlatformConfiguration(platformConfigJson);
            if (rc != Core::ERROR_NONE) {
                LOGERR("GetDeviceModel: GetSystemPlatformConfiguration failed with error code %d, using default value", rc);
                deviceModelJson = R"({"device_model":"UNKNOWN"})";
                return Core::ERROR_NONE;
            }
            Core::JSON::VariantContainer platform;
            platform.FromString(platformConfigJson);
            Core::JSON::VariantContainer out;
            if (platform.HasLabel("model"))
                out["device_model"] = platform["model"];
            else
                out["device_model"] = "UNKNOWN";

            // Filter out null/empty values before converting to string
            Core::JSON::VariantContainer filteredOut;
            FilterNullAndEmptyValues(out, filteredOut);
            filteredOut.ToString(deviceModelJson);
            return Core::ERROR_NONE;
        }

        /**
         * @brief Generates legacy unique identifier using SHA-1 hash
         * @param appId The application identifier
         * @param id The base identifier to combine
         * @param magic Application-specific magic string for uniqueness
         * @return SHA-1 hash string in lowercase hexadecimal format
         * 
         * Creates a unique identifier by concatenating the three input parameters and computing
         * their SHA-1 hash. Used for backward compatibility with legacy systems.
         */
        std::string Badger::GetLegacyUID(const std::string& appId, const std::string& id, const std::string& magic) {
            // Concatenate inputs (no separator)
            std::string input = appId + id + magic;

            // Compute SHA-1
            unsigned char hash[SHA_DIGEST_LENGTH];
            SHA1(reinterpret_cast<const unsigned char*>(input.c_str()), input.length(), hash);

            // Convert to lowercase hex string
            std::ostringstream oss;
            for (int i = 0; i < SHA_DIGEST_LENGTH; ++i) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(hash[i]);
            }

            return oss.str();
        };

        /**
         * @brief Retrieves the unique device identifier
         * @param context Gateway context containing application information
         * @param deviceId Output string containing device ID
         * @param skipAuthorization Whether to skip default authorization check
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the X-Device-Id from the authentication service.
         * Requires DATA_deviceId permission when skipAuthorization is false (default).
         * When skipAuthorization is true, the caller is responsible for performing
         * appropriate permission checks before calling this function. This pattern is
         * typically used by wrapper functions that have already validated caller
         * permissions for equivalent or broader access (e.g., API_Dial_operations).
         */
        Core::hresult Badger::GetXDeviceId(const Exchange::GatewayContext& context, std::string& deviceId, const bool skipAuthorization) {
            deviceId.clear();

            if (!skipAuthorization) {
                Core::hresult rc = AuthorizeDataField(context.appId, "DATA_deviceId");
                if (rc != Core::ERROR_NONE)
                    return rc;
            }

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto auth = mDelegateFactory->getDelegate<AuthServiceDelegate>();
            if (!auth)
                return Core::ERROR_UNAVAILABLE;

            Core::hresult idResult = auth->GetXDeviceId(deviceId);
            if (idResult != Core::ERROR_NONE || deviceId.empty()) {
                if (idResult != Core::ERROR_NONE) {
                    LOGERR("GetDeviceId: GetXDeviceId failed with error code %d, using default value", idResult);
                } else {
                    LOGWARN("GetDeviceId: GetXDeviceId returned empty ID, using default value");
                }
                deviceId = "UNKNOWN";
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves basic device identifier by delegating to GetXDeviceId
         * @param context Gateway context containing application information
         * @param deviceIdJson Output string containing device identifier
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Simple wrapper function that calls GetXDeviceId to obtain the device identifier.
         */
        Core::hresult Badger::GetDeviceId(const Exchange::GatewayContext& context, std::string& deviceIdJson) {
            deviceIdJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Dial_operations");
            if (rc != Core::ERROR_NONE)
                return rc;

            bool skipDefaultAuthorization = true;
            std::string deviceId;
            rc = GetXDeviceId(context, deviceId, skipDefaultAuthorization);
            if (rc != Core::ERROR_NONE) {
                LOGERR("GetDeviceId: GetXDeviceId failed with error code %d", rc);
                return rc;
            }

            Core::JSON::VariantContainer deviceIdObj;
            deviceIdObj["deviceId"] = deviceId;
            deviceIdObj.ToString(deviceIdJson);
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves account identifier associated with the application
         * @param context Gateway context containing application information
         * @param accountId Output string containing account identifier
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Delegates to AuthServiceDelegate for account ID retrieval using app context.
         * Requires DATA_accountId permission.
         */
        Core::hresult Badger::GetAccountId(const Exchange::GatewayContext& context, std::string& accountId) {
            accountId.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_accountId");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto authService = mDelegateFactory->getDelegate<AuthServiceDelegate>();
            if (!authService)
                return Core::ERROR_UNAVAILABLE;
            if (authService->GetAccountId(accountId) != Core::ERROR_NONE) {
                LOGERR("GetAccountId failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }

        // ----------------- PrivacySettings -----------------

        /**
         * @brief Retrieves privacy settings for the device
         * @param context The gateway context containing request information
         * @param privacySettingsJson Output JSON string containing privacy settings
         * @return Core::ERROR_NONE on success, error code on failure
         */
        Core::hresult Badger::GetPrivacySettings(const Exchange::GatewayContext& context, Core::JSON::VariantContainer& privacySettings) {
            privacySettings.Clear();

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;

            // get advertising delegate
            auto advertising = mDelegateFactory->getDelegate<AdvertisingDelegate>();
            if (!advertising)
                return Core::ERROR_UNAVAILABLE;

            std::string advertisingId;
            advertising->AdvertisingId(context.appId, advertisingId);
            Core::JSON::VariantContainer adIdObj;
            adIdObj.FromString(advertisingId);
            
            // Validate that the "lmt" field exists in the advertising ID response
            std::string lmtValue;
            if (adIdObj.HasLabel("lmt")) {
                lmtValue = adIdObj["lmt"].String();
            } else {
                LOGWARN("GetPrivacySettings: 'lmt' field missing in advertising ID response, using default value '0'");
                lmtValue = "0";  // Default to not limiting ad tracking if field is missing
            }
            
            privacySettings["lmt"] = lmtValue;

            if (lmtValue == "0") {
                privacySettings["us_privacy"] = "1-N-";
            } else {
                privacySettings["us_privacy"] = "1-Y-";
            }

            return Core::ERROR_NONE;
        }

        // ----------------- DeviceInfo -----------------

        /**
         * @brief Retrieves comprehensive device information
         * @param appId The application identifier requesting the information
         * @param deviceInfoJson Output JSON string containing complete device information
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns a comprehensive object containing device capabilities, privacy settings,
         * device identifiers, account information, and other device metadata.
         * Requires API_UserData_deviceinfo permission.
         */
        Core::hresult Badger::DeviceInfo(const Exchange::GatewayContext& context, std::string& deviceInfoJson) {
            deviceInfoJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_UserData_deviceinfo");
            if (rc != Core::ERROR_NONE)
                return rc;

            Core::JSON::VariantContainer info;
            std::string dcJson;
            Core::hresult dcResult = DeviceCapabilities(context, dcJson);
            if (dcResult != Core::ERROR_NONE || dcJson.empty()) {
                if (dcResult != Core::ERROR_NONE) {
                    LOGERR("DeviceInfo: DeviceCapabilities failed with error code %d, using default value", dcResult);
                } else {
                    LOGWARN("DeviceInfo: DeviceCapabilities returned empty JSON, using default value");
                }
                dcJson =
                        R"({"deviceType":"UNKNOWN","isWifiDevice":"false","videoDimensions":[1920,1080],"webBrowser":{"browserType":"UNKNOWN"},"nativeDimensions":[3840,2160],"supportsTrueSD":false})";
            }
            Core::JSON::VariantContainer dcVC;
            dcVC.FromString(dcJson);

            Core::JSON::VariantContainer privacySettings;
            GetPrivacySettings(context, privacySettings);
            info["privacySettings"] = privacySettings;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;

            std::string platformJson;
            Core::hresult platformResult = system->GetSystemPlatformConfiguration(platformJson);
            if (platformResult != Core::ERROR_NONE) {
                LOGERR("DeviceInfo: GetSystemPlatformConfiguration failed with error code %d, using default values", platformResult);
                platformJson = R"({"account_id":"UNKNOWN","device_id":"UNKNOWN"})";
            }
            Core::JSON::VariantContainer platform;
            platform.FromString(platformJson);

            info["deviceCapabilities"] = dcVC;

            // use AuthService to get deviceId and accountId - apply graceful degradation
            std::string deviceId;
            if (GetXDeviceId(context, deviceId) == Core::ERROR_NONE) {
                info["deviceId"] = deviceId;
            }
            // Note: Omit deviceId if DATA_deviceId authorization fails in GetXDeviceId

            std::string accountId;
            if (GetAccountId(context, accountId) == Core::ERROR_NONE) {
                info["accountId"] = accountId;
            }
            // Note: Omit accountId if authorization fails

            std::string zipCode;
            if (GetZipCode(context, zipCode) == Core::ERROR_NONE) {
                info["zipcode"] = zipCode;
            }
            // Note: Omit zipcode if authorization fails

            std::string partnerId;
            if (GetPartnerId(context, partnerId) == Core::ERROR_NONE) {
                info["partnerId"] = partnerId;
            }
            // Note: Omit partnerId if authorization fails

            std::string timeZone;
            if (GetTimeZone(context, timeZone) == Core::ERROR_NONE) {
                info["timeZone"] = timeZone;
            }
            // Note: Omit timeZone if authorization fails

            info["timezone"] = "-5:00";

            info["userExperience"] = "1003";  // Hardcoded in FbEntos
            // As per Ripple -
            // https://github.com/comcast-firebolt/firebolt-devices/blob/a30076504b7321caafa905c349acbab1c4401418/rdke/northamerica/badger/badger.extn.json#L24
            const std::string magic = "5dA6Sg34jdG0PhXf";
            info["deviceHash"] = GetLegacyUID(context.appId, info["deviceId"], magic);
            info["receiverId"] = info["deviceHash"];  // As per Ripple

            if (info.HasLabel("accountId")) {
                std::string accountIdStr = info["accountId"].String();
                info["householdId"] = GetLegacyUID(context.appId, accountIdStr, magic);
            }
            // Note: Omit householdId if accountId is not available

            // Filter out null/empty values before converting to string
            Core::JSON::VariantContainer filteredInfo;
            FilterNullAndEmptyValues(info, filteredInfo);
            filteredInfo.ToString(deviceInfoJson);
            return Core::ERROR_NONE;
        }

        // ----------------- DeviceCapabilities -----------------

        /**
         * @brief Retrieves detailed device capabilities information
         * @param appId The application identifier requesting the information
         * @param deviceCapabilitiesJson Output JSON string containing device capabilities
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns comprehensive device capabilities including display resolutions,
         * browser information, HDR/HDCP support, audio capabilities, and device type.
         * Requires API_DeviceCapabilities_deviceCapabilities permission.
         * 
         */
        Core::hresult Badger::DeviceCapabilities(const Exchange::GatewayContext& context, std::string& deviceCapabilitiesJson) {
            deviceCapabilitiesJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_DeviceCapabilities_deviceCapabilities");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            auto display = mDelegateFactory->getDelegate<DisplaySettingsDelegate>();
            if (!system || !display)
                return Core::ERROR_UNAVAILABLE;

            Core::JSON::VariantContainer caps;

            std::string platformJson;
            Core::hresult platformResult = system->GetSystemPlatformConfiguration(platformJson);
            if (platformResult != Core::ERROR_NONE) {
                LOGERR("DeviceCapabilities: GetSystemPlatformConfiguration failed with error code %d, using default values", platformResult);
                platformJson = R"({"device_type":"UNKNOWN","model":"UNKNOWN","supports_true_sd":false,"browser_type":"UNKNOWN","browser_version":"UNKNOWN","browser_user_agent":"UNKNOWN"})";
            }
            Core::JSON::VariantContainer platform;
            platform.FromString(platformJson);

            Core::JSON::VariantContainer webBrowser;
            if (platform.HasLabel("browser_type"))
                webBrowser["browserType"] = platform["browser_type"];
            else
                webBrowser["browserType"] = "UNKNOWN";
            if (platform.HasLabel("browser_version"))
                webBrowser["version"] = platform["browser_version"];
            else
                webBrowser["version"] = "UNKNOWN";
            if (platform.HasLabel("browser_user_agent"))
                webBrowser["userAgent"] = platform["browser_user_agent"];
            else
                webBrowser["userAgent"] = "UNKNOWN";

            Core::JSON::ArrayType<Core::JSON::Variant> nativeDimensionsArray;
            if (GetNativeDimensions(context, nativeDimensionsArray) == Core::ERROR_NONE) {
                caps["nativeDimensions"] = nativeDimensionsArray;
            }
            // Note: Omit nativeDimensions if authorization fails

            Core::JSON::ArrayType<Core::JSON::Variant> videoDimensionsArray;
            if (GetVideoDimensions(context, videoDimensionsArray) == Core::ERROR_NONE) {
                caps["videoDimensions"] = videoDimensionsArray;
            }
            // Note: Omit videoDimensions if authorization fails

            Core::JSON::VariantContainer audioModes;
            if (GetAudioModeStatus(context, audioModes) == Core::ERROR_NONE) {
                caps["audioModes"] = audioModes;
            }
            // Note: Omit audioModes from response if authorization fails rather than failing entire request

            Core::JSON::VariantContainer hdr;
            if (GetHDRStatus(context, hdr) == Core::ERROR_NONE) {
                caps["hdr"] = hdr;
            }
            // Note: Omit hdr from response if authorization fails rather than failing entire request

            Core::JSON::VariantContainer hdcp;
            if (GetHDCPStatus(context, hdcp) == Core::ERROR_NONE) {
                caps["hdcp"] = hdcp;
            }
            // Note: Omit hdcp from response if authorization fails rather than failing entire request

            std::string deviceType;
            if (GetDeviceType(context, deviceType) == Core::ERROR_NONE && !deviceType.empty()) {
                caps["deviceType"] = deviceType;
            }

            std::string wifiJson;
            if (GetWiFiStatus(context, wifiJson) == Core::ERROR_NONE) {
                Core::JSON::VariantContainer wifiVC;
                wifiVC.FromString(wifiJson);
                if (wifiVC["is_wifi_device"].IsSet()) {
                    caps["isWifiDevice"] = wifiVC["is_wifi_device"].Boolean();
                } else {
                    caps["isWifiDevice"] = false;
                }
            }
            // Note: Omit isWifiDevice if authorization fails

            caps["webBrowser"] = webBrowser;
            bool supportsTrueSd = platform.HasLabel("supports_true_sd") && platform["supports_true_sd"].Boolean();
            caps["supportsTrueSD"] = platform["supports_true_sd"].Boolean();

            if (platform.HasLabel("model"))
                caps["model"] = platform["model"];
            else
                caps["model"] = "UNKNOWN";

            std::string stbVersion;
            if (GetSTBVersion(context, stbVersion) == Core::ERROR_NONE) {
                caps["receiverPlatform"] = stbVersion;
            }
            // Note: Omit receiverPlatform if authorization fails

            std::string makeAndModel;
            if (GetMakeAndModel(context, makeAndModel) == Core::ERROR_NONE) {
                caps["deviceMakeModel"] = makeAndModel;
            }
            // Note: Omit deviceMakeModel if authorization fails

            std::string receiverVersion;
            if (GetReceiverVersion(context, receiverVersion) == Core::ERROR_NONE) {
                caps["receiverVersion"] = receiverVersion;
            }
            
            caps.ToString(deviceCapabilitiesJson);
            return Core::ERROR_NONE;
        }

        // ----------------- Network -----------------

        /**
         * @brief Retrieves network connectivity status information
         * @param appId The application identifier requesting the information
         * @param connectivityJson Output JSON string containing network status
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the current network connectivity status and active network interface.
         * Requires API_Network_networkConnectivity permission.
         */
        Core::hresult Badger::NetworkConnectivity(const Exchange::GatewayContext& context, std::string& connectivityJson) {
            connectivityJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Network_networkConnectivity");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto network = mDelegateFactory->getDelegate<NetworkDelegate>();
            if (!network)
                return Core::ERROR_UNAVAILABLE;

            network->GetNetworkConnectivity(connectivityJson);
            LOGINFO("NetworkConnectivity: %s", connectivityJson.c_str());

            // Filter out null/empty values from the JSON response
            FilterNullAndEmptyValues(connectivityJson);

            return Core::ERROR_NONE;
        }

        // ----------------- Lifecycle -----------------

        /**
         * @brief Initiates application shutdown sequence
         * @param context Gateway context containing application information
         * @param result Output string containing the method response
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Requests the application to shutdown gracefully through LifecycleDelegate.
         * Requires API_Navigation_shutdown permission.
         */
        Core::hresult Badger::Shutdown(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Navigation_shutdown");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto lifecycle = mDelegateFactory->getDelegate<LifecycleDelegate>();
            if (!lifecycle)
                return Core::ERROR_UNAVAILABLE;

            return lifecycle->Shutdown(context, result);
        }

        /**
         * @brief Shows a toaster notification on screen
         * @param context Gateway context containing application information
         * @param result Output string for operation result
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Displays a brief notification message to the user through LifecycleDelegate.
         * Requires API_Notification_showToaster permission.
         */
        Core::hresult Badger::ShowToaster(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Notification_showToaster");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto lifecycle = mDelegateFactory->getDelegate<LifecycleDelegate>();
            if (!lifecycle)
                return Core::ERROR_UNAVAILABLE;

            return lifecycle->ShowToaster(context, result);
        }

        /**
         * @brief Dismisses application loading screen
         * @param context Gateway context containing application information
         * @param result Output string containing the method response
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Signals that the application is ready and loading screen should be dismissed through LifecycleDelegate.
         * Requires API_Navigation_dismissLoadingScreen permission.
         */
        Core::hresult Badger::DismissLoadingScreen(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Navigation_dismissLoadingScreen");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto lifecycle = mDelegateFactory->getDelegate<LifecycleDelegate>();
            if (!lifecycle)
                return Core::ERROR_UNAVAILABLE;

            return lifecycle->DismissLoadingScreen(context, result);
        }

        // ----------------- Simple data access -----------------

        /**
         * @brief Retrieves the user-friendly device name
         * @param appId The application identifier requesting the information
         * @param deviceNameJson Output JSON string containing device name
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the friendly name assigned to the device (e.g., "Living Room").
         * Requires API_Dial_operations permission.
         */
        Core::hresult Badger::GetDeviceName(const Exchange::GatewayContext& context, std::string& deviceNameJson) {
            deviceNameJson.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Dial_operations");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto system = mDelegateFactory->getDelegate<SystemDelegate>();
            if (!system)
                return Core::ERROR_UNAVAILABLE;
            std::string name;
            Core::hresult fnResult = system->GetFriendlyName(context, name);
            if (fnResult != Core::ERROR_NONE || name.empty()) {
                if (fnResult != Core::ERROR_NONE) {
                    LOGERR("GetDeviceName: GetFriendlyName failed with error code %d, using default value", fnResult);
                } else {
                    LOGWARN("GetDeviceName: GetFriendlyName returned empty name, using default value");
                }
                name = "Living Room";
            }
            
            Core::JSON::VariantContainer response;
            response["deviceName"] = name;

            response.ToString(deviceNameJson);

            LOGINFO("GetDeviceName: returning device name '%s'", deviceNameJson.c_str());
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the localization postal code information
         * @param appId The application identifier requesting the information
         * @param postalCodeJson Output JSON string containing postal code
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the postal/country code for localization purposes.
         * Requires DATA_zipCode permission.
         */
        Core::hresult Badger::GetLocalizationPostalCode(const Exchange::GatewayContext& context, std::string& postalCode) {
            postalCode.clear();

            return GetZipCode(context, postalCode);
        }

        // ---------- Advertising ----------

        /**
         * @brief Retrieves the application's app store identifier
         * @param appId The application identifier requesting the information
         * @param result Output string containing the app store bundle ID
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the bundle identifier used in app stores for the given application.
         * Requires API_Advertising_advertisingId permission.
         */
        Core::hresult Badger::AppStoreId(const Exchange::GatewayContext& context, std::string& result) {
            result.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Advertising_advertisingId");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto adv = mDelegateFactory->getDelegate<AdvertisingDelegate>();
            if (!adv)
                return Core::ERROR_UNAVAILABLE;

            std::string bundleId;
            Core::hresult bundleResult = adv->GetAppBundleId(context.appId, bundleId);
            if (bundleResult != Core::ERROR_NONE || bundleId.empty()) {
                if (bundleResult != Core::ERROR_NONE) {
                    LOGERR("AppStoreId: GetAppBundleId failed with error code %d, using default value", bundleResult);
                } else {
                    LOGWARN("AppStoreId: GetAppBundleId returned empty bundle ID, using default value");
                }
                bundleId = "UNKNOWN";
            }
            result = "\"" + bundleId + "\"";
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the limit ad tracking preference
         * @param appId The application identifier requesting the information
         * @param limitAdTracking Output boolean indicating if ad tracking is limited
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns whether the user has opted to limit ad tracking for this application.
         * Requires API_Advertising_advertisingId permission.
         */
        Core::hresult Badger::LimitAdTracking(const Exchange::GatewayContext& context, bool& limitAdTracking) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Advertising_advertisingId");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto adv = mDelegateFactory->getDelegate<AdvertisingDelegate>();
            if (!adv)
                return Core::ERROR_UNAVAILABLE;
            Core::hresult adResult = adv->LimitAdTracking(context.appId, limitAdTracking);
            if (adResult != Core::ERROR_NONE) {
                LOGERR("LimitAdTracking: LimitAdTracking failed with error code %d, using default value", adResult);
                limitAdTracking = false;
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves device advertising attributes
         * @param appId The application identifier requesting the information
         * @param result Output JSON string containing device ad attributes
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns device-specific attributes used for advertising targeting.
         * Requires API_AdPlatform_operations permission.
         */
        Core::hresult Badger::DeviceAdAttributes(const Exchange::GatewayContext& context, std::string& result) {
            result.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_AdPlatform_operations");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto adv = mDelegateFactory->getDelegate<AdvertisingDelegate>();
            if (!adv)
                return Core::ERROR_UNAVAILABLE;
            Core::hresult adResult = adv->DeviceAdAttributes(context.appId, result);
            if (adResult != Core::ERROR_NONE) {
                LOGERR("DeviceAdAttributes: DeviceAdAttributes failed with error code %d, using default value", adResult);
                result = "";
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves the advertising identifier (IFA - Identifier for Advertising)
         * @param appId The application identifier requesting the information
         * @param advertisingId Output JSON string containing advertising ID details
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns the device's advertising identifier, type, and limit tracking status.
         * Requires API_Advertising_advertisingId permission.
         */
        Core::hresult Badger::XIFA(const Exchange::GatewayContext& context, std::string& advertisingId) {
            advertisingId.clear();
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Advertising_advertisingId");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto adv = mDelegateFactory->getDelegate<AdvertisingDelegate>();
            if (!adv)
                return Core::ERROR_UNAVAILABLE;

            std::string raw;
            adv->AdvertisingId(context.appId, raw);

            Core::JSON::VariantContainer src;
            src.FromString(raw);
            Core::JSON::VariantContainer out;
            std::string ifa = DelegateUtils::GetStringSafe(src, "ifa");
            if (ifa.empty())
                ifa = "UNKNOWN";
            out["ifa"] = ifa;
            if (src.HasLabel("ifa_type")) {
                out["ifaType"] = src["ifa_type"];
                out["ifa_type"] = src["ifa_type"];
            }
            if (src.HasLabel("lmt"))
                out["lmt"] = src["lmt"];

            // Filter out null/empty values before converting to string
            Core::JSON::VariantContainer filteredOut;
            FilterNullAndEmptyValues(out, filteredOut);
            filteredOut.ToString(advertisingId);
            return Core::ERROR_NONE;
        }

        /**
         * @brief Initializes advertising object with specified options
         * @param appId The application identifier requesting initialization
         * @param options JSON string containing initialization options (must include 'coppa' field)
         * @param result Output JSON string containing initialization result
         * @return Core::ERROR_NONE on success, Core::ERROR_BAD_REQUEST if options invalid
         * 
         * Initializes advertising services with provided configuration options.
         * Requires API_Advertising_advertisingId permission.
         */
        Core::hresult Badger::InitObject(const Exchange::GatewayContext& context, const std::string& options, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Advertising_advertisingId");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto adv = mDelegateFactory->getDelegate<AdvertisingDelegate>();
            if (!adv)
                return Core::ERROR_UNAVAILABLE;

            Core::JSON::VariantContainer optVC;
            if (!options.empty()) {
                optVC.FromString(options);
            }
            if (!optVC.IsSet() || !optVC.HasLabel("coppa")) {
                result = "{}";
                return Core::ERROR_BAD_REQUEST;
            }
            Core::hresult res = adv->InitObject(context.appId, options, result);
            if (res != Core::ERROR_NONE) {
                result = "{}";
                return res;
            }
            return Core::ERROR_NONE;
        }

        // ---------- Discovery ----------

        /**
         * @brief Links media events to user account
         * @param appId The application identifier requesting the link
         * @param payload JSON string containing media event details
         * @param result Output JSON string containing link result
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Associates media consumption events with the user's account for tracking and recommendations.
         * Requires API_AccountLinkService_mediaEventAccountLink permission.
         */
        Core::hresult Badger::MediaEventAccountLink(const Exchange::GatewayContext& context, const std::string& payload, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_AccountLinkService_mediaEventAccountLink");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto discover = mDelegateFactory->getDelegate<DiscoveryDelegate>();
            if (!discover)
                return Core::ERROR_UNAVAILABLE;
            if (discover->MediaEventAccountLink(context.appId, payload, result) != Core::ERROR_NONE) {
                result = "{}";
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Links entitlements to user account
         * @param appId The application identifier requesting the link
         * @param payload JSON string containing entitlements details (requires 'type' and 'action' fields)
         * @param result Output JSON string containing link result
         * @return Core::ERROR_NONE on success, Core::ERROR_BAD_REQUEST if payload invalid
         * 
         * Associates user entitlements and subscription information with their account.
         */
        Core::hresult Badger::EntitlementsAccountLink(const Exchange::GatewayContext& context, const std::string& payload, std::string& result) {
            Core::JSON::VariantContainer root;
            root.FromString(payload);
            std::string type = DelegateUtils::GetStringSafe(root, "type");
            std::string action = DelegateUtils::GetStringSafe(root, "action");
            if (type.empty() || action.empty()) {
                result = "{}";
                return Core::ERROR_BAD_REQUEST;
            }
            result = "{}";
            return Core::ERROR_NONE;
        }

        // ---------- Simple stubs ----------
        Core::hresult Badger::NavigateToEntityPage(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Navigation_entityPage");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto discovery = mDelegateFactory->getDelegate<DiscoveryDelegate>();
            if (!discovery)
                return Core::ERROR_UNAVAILABLE;

            // Create entity intent for discovery launch
            Core::JSON::VariantContainer entityIntent;
            entityIntent["action"] = "entity";
            Core::JSON::VariantContainer discoveryContext;
            discoveryContext["entityId"] = "default_entity";  // Placeholder - should be parameterized
            entityIntent["context"] = discoveryContext;

            std::string intentStr;
            entityIntent.ToString(intentStr);

            return discovery->Launch(context.appId, intentStr, result);
        }

        /**
         * @brief Navigates to full-screen video player interface
         * @param context Gateway context containing application information
         * @param result Output JSON containing navigation result
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Initiates full-screen video playback interface through DiscoveryDelegate.
         * Creates a playback intent with default parameters and launches it.
         * Requires API_Navigation_fullScreenVideo permission.
         */
        Core::hresult Badger::NavigateToFullScreenVideo(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Navigation_fullScreenVideo");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto discovery = mDelegateFactory->getDelegate<DiscoveryDelegate>();
            if (!discovery)
                return Core::ERROR_UNAVAILABLE;

            // Create playback intent for discovery launch
            Core::JSON::VariantContainer playbackIntent;
            playbackIntent["action"] = "playback";
            Core::JSON::VariantContainer discoveryContext;
            discoveryContext["contentId"] = "default_video";  // Placeholder - should be parameterized
            playbackIntent["context"] = discoveryContext;

            std::string intentStr;
            playbackIntent.ToString(intentStr);

            return discovery->Launch(context.appId, intentStr, result);
        }

        /**
         * @brief Navigates to company information and support page
         * @param context Gateway context containing application information
         * @param result Output JSON containing navigation result
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Initiates navigation to company page through DiscoveryDelegate interface.
         * Creates a section intent with company-specific parameters and launches it.
         * Requires API_Navigation_companyPage permission.
         */
        Core::hresult Badger::NavigateToCompanyPage(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Navigation_companyPage");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto discovery = mDelegateFactory->getDelegate<DiscoveryDelegate>();
            if (!discovery)
                return Core::ERROR_UNAVAILABLE;

            // Create section intent for discovery launch
            Core::JSON::VariantContainer sectionIntent;
            sectionIntent["action"] = "section";
            Core::JSON::VariantContainer discoveryContext;
            discoveryContext["sectionId"] = "company_section";  // Placeholder - should be parameterized
            sectionIntent["context"] = discoveryContext;

            std::string intentStr;
            sectionIntent.ToString(intentStr);

            return discovery->Launch(context.appId, intentStr, result);
        }

        /**
         * @brief Retrieves application launch payload data
         * @param context Gateway context containing application information
         * @param payloadJson Output JSON containing launch payload
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Returns launch parameters and payload data for application initialization.
         * Currently returns empty JSON object as placeholder for future implementation.
         */
        Core::hresult Badger::GetPayload(const Exchange::GatewayContext& context, std::string& payloadJson) {
            payloadJson = "{}";

            Core::hresult rc = AuthorizeDataField(context.appId, "API_Dial_operations");
            if (rc != Core::ERROR_NONE)
                return rc;
                
            return Core::ERROR_NONE;
        }
        // Core::hresult Badger::OnLaunch(const Exchange::GatewayContext& context, std::string& result) {
        //     result = "{}";
        //     return Core::ERROR_NONE;
        // }

        /**
         * @brief Registers or unregisters launch event callbacks
         * @param appId The application identifier requesting the registration
         * @param payload JSON string containing onLaunchCallback boolean
         * @param result Output JSON string containing listener response
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Handles registration/unregistration of launch event listeners for second screen functionality.
         * The method processes the onLaunchCallback parameter to determine whether to listen for launch events.
         */
        Core::hresult Badger::OnLaunch(const Exchange::GatewayContext& context, const std::string& payload, std::string& result) {
            result.clear();

            Core::hresult rc = AuthorizeDataField(context.appId, "API_Dial_operations");
            if (rc != Core::ERROR_NONE)
                return rc;

            if (!mDelegateFactory) {
                LOGERR("OnLaunch: DelegateFactory not available for appId=%s", context.appId.c_str());
                return Core::ERROR_UNAVAILABLE;
            }

            auto lifecycle = mDelegateFactory->getDelegate<LifecycleDelegate>();
            if (!lifecycle) {
                LOGERR("OnLaunch: LifecycleDelegate not available for appId=%s", context.appId.c_str());
                return Core::ERROR_UNAVAILABLE;
            }

            return lifecycle->OnLaunch(context, payload, result);
        }
        /**
         * @brief Retrieves device settings based on requested keys
         * @param appId The application identifier requesting settings
         * @param payload JSON string containing array of setting keys to retrieve
         * @param result Output JSON string containing requested settings values
         * @return Always returns Core::ERROR_NONE
         * 
         * Supports various settings keys including CC_STATE, VOICE_GUIDANCE_STATE,
         * DisplayPersonalizedRecommendations, RememberWatchedPrograms, friendly_name, etc.
         */
        Core::hresult Badger::Settings(const Exchange::GatewayContext& context, const std::string& payload, std::string& result) {
            result.clear();

            // Parse payload to extract params.keys array
            Core::JSON::VariantContainer request;
            if (!payload.empty()) {
                request.FromString(payload);
            }

            Core::JSON::ArrayType<Core::JSON::Variant> keys;
            if (request.HasLabel("params") && request["params"].Content() == Core::JSON::Variant::type::OBJECT) {
                Core::JSON::VariantContainer params = request["params"].Object();
                if (params.HasLabel("keys") && params["keys"].Content() == Core::JSON::Variant::type::ARRAY) {
                    keys = params["keys"].Array();
                }
            } else if (request.HasLabel("keys") && request["keys"].Content() == Core::JSON::Variant::type::ARRAY) {
                keys = request["keys"].Array();
            }

            // Prepare delegates
            auto userSettings = (mDelegateFactory ? mDelegateFactory->getDelegate<UserSettingsDelegate>() : nullptr);
            auto privacy = (mDelegateFactory ? mDelegateFactory->getDelegate<PrivacyDelegate>() : nullptr);
            auto system = (mDelegateFactory ? mDelegateFactory->getDelegate<SystemDelegate>() : nullptr);

            Core::JSON::VariantContainer merged;
            
            // Helper to set {"<key>":{"enabled":<bool>}} from delegate JSON {"enabled":<bool>}
            auto setEnabledFromJson = [&](const std::string& outKey, const std::string& srcJson) {
                Core::JSON::VariantContainer enabledObj;
                if (!srcJson.empty()) {
                    enabledObj.FromString(srcJson);
                }
                // If missing/failed, default to false to preserve shape
                if (!enabledObj.HasLabel("enabled")) {
                    enabledObj["enabled"] = Core::JSON::Boolean(false);
                }
                merged.Set(outKey.c_str(), enabledObj);
            };

            // Iterate request keys and populate shapes
            Core::JSON::ArrayType<Core::JSON::Variant>::Iterator it(keys.Elements());
            while (it.Next()) {
                const std::string key = it.Current().String();

                if (key == "CC_STATE" || key == "ShowClosedCapture") {
                    std::string authField = (key == "CC_STATE") ? "DATA_CC_STATE" : "DATA_ShowClosedCapture";
                    Core::hresult rc = AuthorizeDataField(context.appId, authField.c_str());
                    if (Core::ERROR_NONE != rc) {
                        setEnabledFromJson(key, "{}");
                        continue;
                    }
                    std::string json;
                    if (userSettings && userSettings->GetCaptionsEnabled(json) == Core::ERROR_NONE) {
                        setEnabledFromJson(key, json);
                    } else {
                        setEnabledFromJson(key, "{}");
                    }
                } else if (key == "VOICE_GUIDANCE_STATE" || key == "TextToSpeechEnabled2") {
                    std::string authField = (key == "VOICE_GUIDANCE_STATE") ? "DATA_VOICE_GUIDANCE_STATE" : "DATA_TextToSpeechEnabled2";
                    Core::hresult rc = AuthorizeDataField(context.appId, authField.c_str());
                    if (Core::ERROR_NONE != rc) {
                        setEnabledFromJson(key, "{}");
                        continue;
                    }
                    std::string json;
                    if (userSettings && userSettings->GetVoiceGuidanceState(json) == Core::ERROR_NONE) {
                        setEnabledFromJson(key, json);
                    } else {
                        setEnabledFromJson(key, "{}");
                    }
                } else if (key == "DisplayPersonalizedRecommendations") {
                    Core::hresult rc = AuthorizeDataField(context.appId, "DATA_DisplayPersonalizedRecommendations");
                    if (rc != Core::ERROR_NONE) {
                        setEnabledFromJson(key, "{}");
                        continue;
                    }
                    std::string json;
                    if (privacy && privacy->GetPersonalizationAllowed(json) == Core::ERROR_NONE) {
                        setEnabledFromJson(key, json);
                    } else {
                        setEnabledFromJson(key, "{}");
                    }
                } else if (key == "RememberWatchedPrograms") {
                    Core::hresult rc = AuthorizeDataField(context.appId, "DATA_RememberWatchedPrograms");
                    if (rc != Core::ERROR_NONE) {
                        setEnabledFromJson(key, "{}");
                        continue;
                    }
                    // Get from privacy delegate
                    std::string json;
                    if (privacy && privacy->GetWatchHistoryAllowed(json) == Core::ERROR_NONE) {
                        setEnabledFromJson(key, json);
                    } else {
                        setEnabledFromJson(key, "{}");
                    }
                } else if (key == "ShareWatchHistoryStatus") {
                    Core::hresult rc = AuthorizeDataField(context.appId, "DATA_ShareWatchHistoryStatus");
                    if (rc != Core::ERROR_NONE) {
                        setEnabledFromJson(key, "{}");
                        continue;
                    }
                    // Get the stored user grant value for DATA_APP_USAGE capability via HandleAppGatewayRequest
                    bool isGranted = false;
                    if (mService != nullptr) {
                        auto launchDelegate = mService->QueryInterfaceByCallsign<Exchange::IAppGatewayRequestHandler>(LAUNCH_DELEGATE_CALLSIGN);
                        if (launchDelegate != nullptr) {
                            static constexpr const char* CAPABILITY_DATA_APP_USAGE = "xrn:firebolt:capability:data:app-usage";
                            std::string launchDelegatePayload = R"({"capability":")" + std::string(CAPABILITY_DATA_APP_USAGE) + R"("})";
                            std::string result;
                            Core::hresult rc = launchDelegate->HandleAppGatewayRequest(context, "usergrants.isallowed", launchDelegatePayload, result);
                            if (rc == Core::ERROR_NONE && result == "true") {
                                isGranted = true;
                            }
                            launchDelegate->Release();
                        } else {
                            LOGERR("Settings: LaunchDelegate IAppGatewayRequestHandler interface not found for ShareWatchHistoryStatus");
                        }
                    }
                    std::string jsonStr = isGranted ? R"({"enabled":true})" : R"({"enabled":false})";
                    setEnabledFromJson(key, jsonStr);
                } else if (key == "friendly_name") {
                    Core::hresult rc = AuthorizeDataField(context.appId, "DATA_friendly_name");
                    if (rc != Core::ERROR_NONE) {
                        setEnabledFromJson(key, "{}");
                        continue;
                    }
                    std::string fnJson;
                    std::string name = "Living room";
                    if (system) {
                        Core::hresult fnResult = system->GetFriendlyName(context, fnJson);
                        if (fnResult == Core::ERROR_NONE) {
                            Core::JSON::VariantContainer obj;
                            obj.FromString(fnJson);
                            if (obj.HasLabel("friendly_name")) {
                                const std::string n = obj["friendly_name"].String();
                                if (!n.empty()) {
                                    name = n;
                                }
                            }
                        } else {
                            LOGERR("Settings: GetFriendlyName failed with error code %d, using default value", fnResult);
                        }
                    }
                    // Wrap with inner quotes as requested: "\"<Name>\""
                    std::string valueWithQuotes = std::string("\"") + name + std::string("\"");
                    Core::JSON::VariantContainer wrap;
                    wrap["value"] = valueWithQuotes;
                    merged["friendly_name"] = wrap;
                } else if (key == "legacyMiniGuide") {
                    LOGDBG("Settings: Legacy Mini Guide status requested");
                } else if (key == "power_save_status") {
                    LOGDBG("Settings: Power Save status requested");
                } else {
                    // Ignore unknown keys to include only requested recognized keys
                    LOGDBG("Settings: Unknown key '%s' requested", key.c_str());
                }
            }

            // Serialize result as the merged object (JSON-RPC wrapping handled upstream)
            // Filter out null/empty values before converting to string
            Core::JSON::VariantContainer filteredMerged;
            FilterNullAndEmptyValues(merged, filteredMerged);
            filteredMerged.ToString(result);
            if (result.empty()) {
                // If no keys provided, return empty object
                result = "{}";
            }
            return Core::ERROR_NONE;
        }

        /**
         * @brief Subscribes to settings change notifications
         * @param context Gateway context containing application information
         * @param result Output JSON containing subscription result
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Establishes subscription for real-time settings change notifications.
         * Currently returns empty JSON object as placeholder for future implementation.
         */
        Core::hresult Badger::SubscribeToSettings(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_UserData_subscribeToSettings");
            if (rc != Core::ERROR_NONE)
                return rc;
            result = "{}";
            return Core::ERROR_NONE;
        }

        /**
         * @brief Links launchpad tile interaction to user account
         * @param context Gateway context containing application information
         * @param payload JSON string containing launchpadTile with contentId
         * @param result Output JSON string (returns empty object)
         * @return Core::ERROR_NONE on success.
         * 
         * Processes launchpad tile interactions and calls WatchNext for content tracking.
         */
        Core::hresult Badger::LaunchpadAccountLink(const Exchange::GatewayContext& context, const std::string& payload, std::string& result) {
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto discovery = mDelegateFactory->getDelegate<DiscoveryDelegate>();
            if (!discovery)
                return Core::ERROR_UNAVAILABLE;

            Core::JSON::VariantContainer payloadVC;
            if (!payload.empty()) {
                payloadVC.FromString(payload);
            }

            std::string contentId;
            if (payloadVC.HasLabel("launchpadTile") && payloadVC["launchpadTile"].Content() == Core::JSON::Variant::type::OBJECT) {
                Core::JSON::VariantContainer lp = payloadVC["launchpadTile"].Object();
                if (lp.HasLabel("contentId")) {
                    contentId = lp["contentId"].String();
                }
            }

            if (!contentId.empty()) {
                std::string tmp;
                (void) discovery->WatchNext(context.appId, contentId, tmp);
                // add debug log
                LOGINFO("launchpadAccountLink: launchpadTile.contentId found, returning %s", tmp.c_str());
            } else {
                LOGWARN("launchpadAccountLink: launchpadTile.contentId missing, returning {}");
            }
            result = "{}";
            return Core::ERROR_NONE;
        }

        /**
         * @brief Performs application authentication and returns success status
         * @param context Gateway context containing application information
         * @param result Output JSON containing authentication result
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Authenticates application and provides success confirmation.
         * Currently returns hardcoded success status for backward compatibility.
         */
        Core::hresult Badger::AppAuth(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "DATA_appAuthData");
            if (rc != Core::ERROR_NONE)
                return rc;
            result = R"({"status":"SUCCESS"})";
            return Core::ERROR_NONE;
        }

        /**
         * @brief Retrieves OAuth bearer token for authenticated API access
         * @param context Gateway context containing application information
         * @param result Output JSON containing error message
         * @return ERROR_NOT_SUPPORTED indicating this capability is not supported
         * 
         * OAuth bearer token functionality is not currently supported.
         * Returns appropriate error message for token session capability.
         * 
         * Security Note: This function checks both required permissions upfront
         * to prevent authorization bypass attacks where an attacker could probe
         * which permissions they have by observing different error responses
         * from sequential authorization checks.
         */
        Core::hresult Badger::OAuthBearerToken(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Auth_OAuthBearerToken");
            if (Core::ERROR_NONE != rc)
                return rc;

            std::string partnerId;
            // Skip authorization since we already checked it above
            Core::hresult partnerRc = GetPartnerId(context, partnerId, true);
            if (Core::ERROR_NONE != partnerRc)
                return partnerRc;

            if ("comcast" == partnerId) {
                Exchange::IOttServices* ottServices = GetOttServices();
                if (nullptr == ottServices) {
                    LOGERR("OttServices interface not available");
                    result = R"({"message": "capability xrn:firebolt:capability:token:session is not supported"})";
                    return Core::ERROR_UNAVAILABLE;
                }

                std::string token;
                if (Core::ERROR_NONE != ottServices->GetAppCIMAToken(context.appId, token)) {
                    LOGERR("GetAppCIMAToken failed for appId=%s", context.appId.c_str());
                    result = R"({"message": "capability xrn:firebolt:capability:token:session is not supported"})";
                    return Core::ERROR_UNAVAILABLE;
                } else {
                    Core::JSON::VariantContainer response;
                    response["access_token"] = token;
                    response["expires_in"] = 3600;  // Default 1 hour expiry
                    response["token_type"] = "Bearer";
                    response["scope"] = "";
                    response["tid"] = "";
                    response.ToString(result);
                    return Core::ERROR_NONE;
                }
            } else {
                LOGWARN("Unsupported partnerId=%s", partnerId.c_str());
                ErrorUtils::NotSupported(result);
                return ERROR_NOT_SUPPORTED;
            }
        }

        /**
         * @brief Refreshes platform authentication token for continued access
         * @param context Gateway context containing application information
         * @param result Output JSON containing refreshed authentication token
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Refreshes expired authentication tokens through OttServices interface.
         * Uses device session ID and content provider information for token renewal.
         */
        Core::hresult Badger::RefreshPlatformAuthToken(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Auth_refreshPlatformAuthToken");
            if (rc != Core::ERROR_NONE)
                return rc;
            result.clear();
            Exchange::IOttServices* ottServices = GetOttServices();
            if (ottServices == nullptr) {
                LOGERR("OttServices interface not available");
                return Core::ERROR_UNAVAILABLE;
            }
            std::string contentProvider = GetAppCatalogId(context.appId);
            std::string deviceSessionId = GetDeviceSessionId(context, context.appId);
            std::string appSessionId = GetAppSessionId(context.appId);
            if (ottServices->GetAppThorToken(context.appId, contentProvider, deviceSessionId, appSessionId, result) != Core::ERROR_NONE) {
                LOGERR("RefreshPlatformAuthToken failed for appId=%s", context.appId.c_str());
                return Core::ERROR_PRIVILIGED_REQUEST;
            }
            result = "\"" + result + "\"";
            LOGINFO("RefreshPlatformAuthToken:  for appId=%s , contentProvider = %s,  deviceSessionId = %s, appSessionId = %s", context.appId.c_str(), contentProvider.c_str(),
                    deviceSessionId.c_str(), appSessionId.c_str());
            return Core::ERROR_NONE;
        }
        Core::hresult Badger::GetXact(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_Auth_getXact");
            if (rc != Core::ERROR_NONE)
                return rc;
            LOGINFO("GetXact called");
            result.clear();
            if (!mDelegateFactory)
                return Core::ERROR_UNAVAILABLE;
            auto authService = mDelegateFactory->getDelegate<AuthServiceDelegate>();
            if (!authService)
                return Core::ERROR_UNAVAILABLE;
            if (authService->GetXact(context.appId, result) != Core::ERROR_NONE) {
                LOGERR("GetXact failed!");
                return Core::ERROR_UNAVAILABLE;
            }
            return Core::ERROR_NONE;
        }
        Core::hresult Badger::LogMoneyBadgerLoaded(const Exchange::GatewayContext& context, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_MetricsService_logMoneyBadgerLoaded");
            if (rc != Core::ERROR_NONE)
                return rc;
            result = "{}";
            return Core::ERROR_NONE;
        }

        /**
         * @brief Handles metrics reporting for various event types
         * @param context Gateway context containing application information
         * @param payload JSON string containing event details and parameters
         * @param result Output string ("true" on success, "false" on failure)
         * @return Core::ERROR_NONE on success, error code on failure
         * 
         * Delegates all metrics handling to MetricsDelegate::HandleMetrics which contains
         * the comprehensive routing logic for all event types and payload formats.
         */
        Core::hresult Badger::MetricsHandler(const Exchange::GatewayContext& context, const std::string& payload, std::string& result) {
            Core::hresult rc = AuthorizeDataField(context.appId, "API_MetricsService_metricsHandler");
            if (rc != Core::ERROR_NONE)
                return rc;
            if (!mDelegateFactory) {
                return Core::ERROR_UNAVAILABLE;
            }
            auto metrics = mDelegateFactory->getDelegate<MetricsDelegate>();
            if (!metrics) {
                return Core::ERROR_UNAVAILABLE;
            }

            // Delegate all parsing and routing logic to MetricsDelegate::HandleMetrics
            return metrics->HandleMetrics(payload, context.appId, result);
        }

        // ----------------- Dispatcher -----------------

        using HandlerFunction = Core::hresult (Badger::*)(const Exchange::GatewayContext&, std::string&);
        using PayloadHandlerFunction = Core::hresult (Badger::*)(const Exchange::GatewayContext&, const std::string&, std::string&);

        /**
         * @brief Main request dispatcher for App Gateway API calls
         * @param context Gateway context containing application information
         * @param method The API method name being called (may include "badger." prefix)
         * @param payload JSON payload for methods that require input parameters
         * @param result Output string containing the method response
         * @return Core::ERROR_NONE on success, Core::ERROR_UNKNOWN_KEY for unsupported methods
         * 
         * Routes incoming API calls to appropriate handler methods based on method name.
         * Uses HashMap-based dispatch for O(1) lookup performance instead of linear search.
         * Supports both no-payload methods and methods requiring input parameters.
         * Handles permission validation and method-specific processing.
         */
        Core::hresult Badger::HandleAppGatewayRequest(const Exchange::GatewayContext& context, const std::string& method, const std::string& payload, std::string& result) {
            LOGTRACE("HandleAppGatewayRequest: method=%s payload=%s appId=%s", method.c_str(), payload.c_str(), context.appId.c_str());

            std::string name = method;
            const std::string prefix = "badger.";
            if (name.size() >= prefix.size() && name.compare(0, prefix.size(), prefix) == 0) {
                name = name.substr(prefix.size());
            }

            // HashMap-based dispatch for no-payload handlers (O(1) lookup)
            static const std::unordered_map<std::string, HandlerFunction> noPayloadHandlers = {{"info", &Badger::DeviceInfo}, {"deviceCapabilities", &Badger::DeviceCapabilities},
                    {"networkConnectivity", &Badger::NetworkConnectivity}, {"getDeviceId", &Badger::GetDeviceId}, {"getDeviceName", &Badger::GetDeviceName},
                    {"localizationPostalCode", &Badger::GetLocalizationPostalCode}, {"getPayload", &Badger::GetPayload}, {"navigateToCompanyPage", &Badger::NavigateToCompanyPage},
                    {"navigateToEntityPage", &Badger::NavigateToEntityPage}, {"navigateToFullScreenVideo", &Badger::NavigateToFullScreenVideo}, {"logMoneyBadgerLoaded", &Badger::LogMoneyBadgerLoaded},
                    {"xifa", &Badger::XIFA}, {"appStoreId", &Badger::AppStoreId}, {"deviceAdAttributes", &Badger::DeviceAdAttributes}, {"getXact", &Badger::GetXact},
                    {"refreshPlatformAuthToken", &Badger::RefreshPlatformAuthToken}, {"subscribeToSettings", &Badger::SubscribeToSettings}, {"appAuth", &Badger::AppAuth},
                    {"OAuthBearerToken", &Badger::OAuthBearerToken}, {"shutdown", &Badger::Shutdown}, {"dismissLoadingScreen", &Badger::DismissLoadingScreen}, {"showToaster", &Badger::ShowToaster}};

            // Fast HashMap lookup for no-payload handlers
            auto it = noPayloadHandlers.find(name);
            if (it != noPayloadHandlers.end()) {
                return (this->*(it->second))(context, result);
            }

            if (name == "limitAdTracking") {
                bool lat = false;
                Core::hresult rc = AuthorizeDataField(context.appId, "API_Advertising_advertisingId");
                if (rc != Core::ERROR_NONE)
                    return rc;
                rc = LimitAdTracking(context, lat);
                if (rc != Core::ERROR_NONE)
                    return rc;
                result = lat ? "true" : "false";
                return Core::ERROR_NONE;
            }

            // HashMap-based dispatch for payload handlers (O(1) lookup)
            static const std::unordered_map<std::string, PayloadHandlerFunction> payloadHandlers = {{"mediaEventAccountLink", &Badger::MediaEventAccountLink}, {"initObject", &Badger::InitObject},
                    {"entitlementsAccountLink", &Badger::EntitlementsAccountLink}, {"metricsHandler", &Badger::MetricsHandler}, {"settings", &Badger::Settings}, {"onLaunch", &Badger::OnLaunch},
                    {"launchpadAccountLink", &Badger::LaunchpadAccountLink}};

            auto payloadIt = payloadHandlers.find(name);
            if (payloadIt != payloadHandlers.end()) {
                return (this->*(payloadIt->second))(context, payload, result);
            }

            ErrorUtils::NotSupported(result);
            LOGERR("Unsupported method: %s", method.c_str());
            return Core::ERROR_UNKNOWN_KEY;
        }

    }  // namespace Plugin
}  // namespace WPEFramework
