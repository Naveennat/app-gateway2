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

#include <cstdlib>
#include <fstream>
#include "OttServicesImplementation.h"

#include <core/Enumerate.h>
 // Use in-module JSONRPC direct link utils
#include "UtilsJsonrpcDirectLink.h"
#include <interfaces/IAuthService.h>

// Token client and cache
#include "TokenClient.h"
#include "TokenCache.h"
#define PARTNER_ID_FILENAME "/opt/www/authService/partnerId3.dat"
#define ACCOUNT_ID_FILENAME "/opt/www/authService/said.dat"
#define DEVICE_ID_FILENAME "/opt/www/authService/xdeviceid.dat"


namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(OttServicesImplementation, 1, 0);

    namespace {
        static constexpr const char* kDefaultPermsEndpoint = "thor-permission.svc.thor.comcast.com";
        static constexpr const char* kDefaultTokenEndpoint = "ott-token-service.svc.thor.comcast.com";
        static constexpr const char* kAuthServiceCallsign = "org.rdk.AuthService";
    }

    OttServicesImplementation::OttServicesImplementation()
        : _service(nullptr)
        , _state(_T("uninitialized"))
        , _perms(nullptr)
        , _permsEndpoint(kDefaultPermsEndpoint)
        , _permsUseTls(true)
        , _token(nullptr)
        , _tokenEndpoint(kDefaultTokenEndpoint)
        , _tokenUseTls(true)
        , _refCount(1) {
    }

    OttServicesImplementation::~OttServicesImplementation() = default;

    uint32_t OttServicesImplementation::Configure(PluginHost::IShell* shell) {

        LOGINFO("Configuring OttServices");
        ASSERT(shell != nullptr);
        // Configure using the provided shell (mirrors Initialize logic; return Core::ERROR_NONE on success)
        _service = shell;
        _service->AddRef();
        Initialize(shell);
        return Core::ERROR_NONE;
    }

    string OttServicesImplementation::Initialize(PluginHost::IShell* service) {
        _service = service;
        _state = _T("ready");

        // Load configuration: PermissionsEndpoint, UseTls
        if (_service != nullptr) {
            Config cfg;
            cfg.FromString(_service->ConfigLine());
            _permsEndpoint = (cfg.PermissionsEndpoint.IsSet() && !cfg.PermissionsEndpoint.Value().empty())
                                ? cfg.PermissionsEndpoint.Value()
                                : std::string(kDefaultPermsEndpoint);
            _permsUseTls = cfg.UseTls.IsSet() ? static_cast<bool>(cfg.UseTls.Value()) : true;

            _tokenEndpoint = (cfg.TokenEndpoint.IsSet() && !cfg.TokenEndpoint.Value().empty())
                                ? cfg.TokenEndpoint.Value()
                                : std::string(kDefaultTokenEndpoint);
            _tokenUseTls = cfg.UseTlsToken.IsSet() ? static_cast<bool>(cfg.UseTlsToken.Value()) : true;
        }
      
        _perms.reset();
        _token.reset();

        LOGINFO("OttServices: Initialize implementation (permsEndpoint=%s, permsTls=%s, tokenEndpoint=%s, tokenTls=%s)",
                _permsEndpoint.c_str(), _permsUseTls ? "true" : "false",
                _tokenEndpoint.c_str(), _tokenUseTls ? "true" : "false");

        // Return empty string on success per Thunder conventions.
        return string();
    }

    void OttServicesImplementation::Deinitialize(PluginHost::IShell* /*service*/) {
        _perms.reset();
        _token.reset();
        if (_service != nullptr) {
            _service->Release();
        }
        _state = _T("deinitialized");
        _service = nullptr;
        LOGINFO("OttServices: Deinitialize implementation");
    }

    
    void OttServicesImplementation::SetPermissionsClient() const {

        LOGINFO("OttServices:  PermissionsClient Initialize  (endpoint=%s, tls=%s)",
                _permsEndpoint.c_str(), _permsUseTls ? "true" : "false");

        // Create permissions client (works even if gRPC disabled; will return ERROR_UNAVAILABLE on use)
        _perms = std::make_shared<PermissionsClient>(_permsEndpoint, _permsUseTls);

        // Warm up cache file load
        OttPermissionCache::Instance();
        _permissionsClientInitialized = true;

        LOGINFO("OttServices: PermissionsClient initialized (endpoint=%s, tls=%s)",
                _permsEndpoint.c_str(), _permsUseTls ? "true" : "false");
    }


    void OttServicesImplementation::SetTokenClient() const {

        LOGINFO("OttServices:  TokenClient Initialize  (endpoint=%s, tls=%s)",
                _tokenEndpoint.c_str(), _tokenUseTls ? "true" : "false");

        // Create token client (works even if gRPC disabled; will return ERROR_UNAVAILABLE on use)
        _token = std::make_shared<TokenClient>(_tokenEndpoint, _tokenUseTls);
        _tokenClientInitialized = true;

        LOGINFO("OttServices: TokenClient initialized (endpoint=%s, tls=%s)",
                _tokenEndpoint.c_str(), _tokenUseTls ? "true" : "false");
    }

    Core::hresult OttServicesImplementation::GetPermissionsFromServer(const string& appId, std::vector<std::string>& permissions) const {
        if(_perms == nullptr)   
        {
            SetPermissionsClient();
            LOGINFO("PermissionsClient initialized");
        }

        if (appId.empty()) {
            LOGERR("bad request (empty appId)");
            return Core::ERROR_BAD_REQUEST;
        }
        if (!_perms) {
            LOGERR("failed - PermissionsClient not initialized");
            return Core::ERROR_UNAVAILABLE;
        }

        LOGINFO("start (appId='%s', endpoint=%s)", appId.c_str(), _perms->Endpoint().c_str());

        std::string token;
        std::string deviceId;
        std::string accountId;
        std::string partnerId;
        if (!CollectAuthMetadata(token, deviceId, accountId, partnerId)) {
            LOGERR("auth metadata collection failed (appId='%s')", appId.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        LOGINFO("fetching permissions (appId='%s')", appId.c_str());

        std::vector<std::string> filters; // none for now
        const uint32_t rc = _perms->EnumeratePermissions(appId, partnerId, token, deviceId, accountId, filters, permissions);
        if (rc != Core::ERROR_NONE) {
            LOGWARN("fetch failed (appId='%s', rc=%u)", appId.c_str(), rc);
        }

        return rc;
    }

    Core::hresult OttServicesImplementation::UpdatePermissions(const string& appId, std::vector<std::string>& permissions) const {

        // Get Permissions from server 
        const uint32_t rc = GetPermissionsFromServer(appId, permissions);

        // Update Cache on a separate thread
        Core::IWorkerPool::Instance().Submit(UpdatePermissionCacheJob::Create(appId, permissions));

        return rc;
    }

    Core::hresult OttServicesImplementation::GetAppPermissions(const string& appId, bool forceNew, Exchange::IOttServices::IStringIterator*& permissions) const {
        LOGINFO("GetAppPermissions called for appId='%s' (forceNew=%s)", appId.c_str(), forceNew ? "true" : "false");

        // Initialize permissions client if not already initialized
        if (_perms == nullptr) {
            SetPermissionsClient();
            LOGINFO("PermissionsClient initialized");
        }

        // Get permissions from cache or fetch new if forceNew is true
        std::vector<string> permissionsList;
        
        if (forceNew) {
            // Force refresh: fetch from upstream and update cache
            if(UpdatePermissions(appId, permissionsList) != Core::ERROR_NONE) {
                LOGERR("Failed to update permissions for appId='%s'", appId.c_str());
                return Core::ERROR_UNAVAILABLE;
            }
        } else {
            // Get from cache
            permissionsList = OttPermissionCache::Instance().GetPermissions(appId);
            
            if (permissionsList.empty()) {
                LOGWARN("No cached permissions for appId='%s', fetching...", appId.c_str());
                if(UpdatePermissions(appId, permissionsList) != Core::ERROR_NONE) {
                    LOGERR("Failed to update permissions for appId='%s'", appId.c_str());
                    return Core::ERROR_UNAVAILABLE;
                }
            }

        }

        LOGINFO("GetPermissions completed for appId='%s', count=%zu", appId.c_str(), permissionsList.size());

        // Create iterator from permissions list
        permissions = Core::Service<RPC::StringIterator>::Create<RPC::IStringIterator>(permissionsList);

        return Core::ERROR_NONE;
    }

    string OttServicesImplementation::GetPartnerId() const
    {
        // Get actual id, as it may change at any time...
        std::ifstream input(PARTNER_ID_FILENAME);
        string line;
        getline(input, line);
        input.close();
        LOGINFO("OttServices: Retrieved partner ID from file");
        return line;
    }

    string OttServicesImplementation::GetAccountId() const
    {        
        // Get actual id, as it may change at any time...
        std::ifstream input(ACCOUNT_ID_FILENAME);
        string line;
        getline(input, line);
        input.close();
        LOGINFO("OttServices: Retrieved account ID from file");
        return line;
    }

    string OttServicesImplementation::GetDeviceId() const
    {        
        // Get actual id, as it may change at any time...
        std::ifstream input(DEVICE_ID_FILENAME);
        string line;
        getline(input, line);
        input.close();
        LOGINFO("OttServices: Retrieved device ID from file");
        return line;
    }

    bool OttServicesImplementation::CollectAuthMetadata(std::string& bearerToken,
                                                        std::string& deviceId,
                                                        std::string& accountId,
                                                        std::string& partnerId) const {
        // Parity note: This implementation mirrors Supporting_Files/DeviceInfo.cpp patterns:
        // - Uses Utils::GetThunderControllerClient to directly invoke AuthService methods.
        // - Validates presence of expected labels and non-empty values.
        // - Requires "expires" when fetching service access token (as in DeviceInfo::GetServiceAccessToken).
        // - Returns false on failure, allowing callers to map to Core::ERROR_UNAVAILABLE consistently.

        LOGINFO("OttServices: CollectAuthMetadata called");
        if (_service == nullptr) {
            LOGERR("OttServices: CollectAuthMetadata called with null service");
            return false;
        }
        Exchange::IAuthService *authservicePlugin = _service->QueryInterfaceByCallsign<Exchange::IAuthService>("org.rdk.AuthService");
        if (authservicePlugin != nullptr) {
            WPEFramework::Exchange::IAuthService::GetServiceAccessTokenResult atRes;
            uint32_t res = authservicePlugin->GetServiceAccessToken(atRes);
            authservicePlugin->Release();
            if (res == Core::ERROR_NONE){
                bearerToken = atRes.token;
                LOGINFO("OttServices: Retrieved bearer token from AuthService plugin");
            }
            else{
                LOGERR("OttServices: GetServiceAccessToken from AuthService plugin failed rc=%u", res);
                return false;
            }
        } else{
            LOGERR("OttServices: QueryInterfaceByCallsign for IAuthService failed");
        }
        deviceId = GetDeviceId();
        accountId = GetAccountId();
        partnerId = GetPartnerId();
        // Final sanity check
        if (bearerToken.empty() || deviceId.empty() || accountId.empty() || partnerId.empty()) {
            LOGERR("OttServices: one or more required AuthService values are empty (token=[REDACTED], device=%s, account=%s, partner=%s)",
                   deviceId.c_str(), accountId.c_str(), partnerId.c_str());
            return false;
        }
        return true;
    }

    Core::hresult OttServicesImplementation::GetAppThorToken(const string& appId,
                                                  const string& contentProvider,
                                                  const string& deviceSessionId,
                                                  const string& appSessionId,
                                                  string& outToken) const {
        LOGINFO("OttServices: GetAppThorToken called (appId='%s')", appId.c_str());
        if(_perms == nullptr)
        {
            SetPermissionsClient();
            LOGINFO("OttServices: PermissionsClient initialized inside GetAppThorToken");
        }

        std::string bearerToken;
        std::string deviceId;
        std::string accountId;
        std::string partnerId;
        if (!CollectAuthMetadata(bearerToken, deviceId, accountId, partnerId)) {
            LOGERR("OttServices: GetAppThorToken auth metadata collection failed (appId='%s')", appId.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        std::string tokenMode = "untrusted"; // as per Ripple code 
        int32_t ttl = 3600; // 1 hour

        const uint32_t rc = _perms->GetThorTokenRPC(appId, contentProvider, deviceSessionId, appSessionId,
                                                 tokenMode, ttl, bearerToken, deviceId, accountId,
                                                 partnerId, outToken);
        if (rc != Core::ERROR_NONE) {
            LOGWARN("OttServices: GetAppThorToken failed (appId='%s', rc=%u)", appId.c_str(), rc);
            return rc;
        }

        LOGINFO("OttServices: GetAppThorToken succeeded (appId='%s')", appId.c_str());
        return Core::ERROR_NONE;
    }

    // Helper: current epoch seconds
    uint64_t OttServicesImplementation::NowEpochSec() {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    }

    bool OttServicesImplementation::FetchSat(std::string& sat, uint64_t& expiryEpoch) const {
        sat.clear();
        expiryEpoch = 0;

        if (_service == nullptr) {
            LOGERR("OttServices: FetchSat called with null service");
            return false;
        }

        Exchange::IAuthService* authService = _service->QueryInterfaceByCallsign<Exchange::IAuthService>(kAuthServiceCallsign);
        if (authService == nullptr) {
            LOGERR("OttServices: Failed to acquire AuthService COM interface for SAT");
            return false;
        }

        Exchange::IAuthService::GetServiceAccessTokenResult result;
        uint32_t rc = authService->GetServiceAccessToken(result);
        authService->Release();

        if (rc != Core::ERROR_NONE || !result.success) {
            LOGERR("OttServices: GetServiceAccessToken failed rc=%u, success=%d", rc, result.success);
            return false;
        }

        sat = result.token;
        expiryEpoch = static_cast<uint64_t>(result.expires);

        return !sat.empty();
    }

    bool OttServicesImplementation::FetchXact(const std::string& appId, std::string& xact, uint64_t& expiryEpoch) const {
        xact.clear();
        expiryEpoch = 0;

        if (_service == nullptr) {
            LOGERR("OttServices: FetchXact called with null service");
            return false;
        }

        Exchange::IAuthService* authService = _service->QueryInterfaceByCallsign<Exchange::IAuthService>(kAuthServiceCallsign);
        if (authService == nullptr) {
            LOGERR("OttServices: Failed to acquire AuthService COM interface for xACT");
            return false;
        }

        Exchange::IAuthService::GetAuthTokenResult result;
        // Note: IAuthService::GetAuthToken takes forceNew and recoverRenewal params
        // Using false for both to get cached token when available
        uint32_t rc = authService->GetAuthToken(false, false, result);
        authService->Release();

        if (rc != Core::ERROR_NONE || !result.success) {
            LOGERR("OttServices: GetAuthToken failed rc=%u, success=%d", rc, result.success);
            return false;
        }

        xact = result.token;
        expiryEpoch = static_cast<uint64_t>(result.expires);

        if (xact.empty()) {
            LOGERR("OttServices: GetAuthToken returned empty xACT");
            return false;
        }
        return true;
    }

    bool OttServicesImplementation::FetchXsct(std::string& xSCT, uint64_t& expiryEpoch) const {
        xSCT.clear();
        expiryEpoch = 0;

        if (_service == nullptr) {
            LOGERR("OttServices: FetchXsct called with null service");
            return false;
        }

        Exchange::IAuthService* authService = _service->QueryInterfaceByCallsign<Exchange::IAuthService>(kAuthServiceCallsign);
        if (authService == nullptr) {
            LOGERR("OttServices: Failed to acquire AuthService COM interface for xACT");
            return false;
        }

        Exchange::IAuthService::GetSessionTokenResult result;
        // Note: IAuthService::GetSessionToken takes forceNew and recoverRenewal params
        // Using false for both to get cached token when available
        uint32_t rc = authService->GetSessionToken(result);
        authService->Release();

        if (rc != Core::ERROR_NONE || !result.success) {
            LOGERR("OttServices: GetSessionToken failed rc=%u, success=%d", rc, result.success);
            return false;
        }

        xSCT = result.token;
        expiryEpoch = static_cast<uint64_t>(result.expires);

        if (xSCT.empty()) {
            LOGERR("OttServices: GetSessionToken returned empty xSCT");
            return false;
        }
        return true;
    }

    Core::hresult OttServicesImplementation::GetAppCIMAToken(const string& appId, string& token) const {
        LOGINFO("OttServices: GetAppCIMAToken called (appId='%s')", appId.c_str());
        if (appId.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        if(_token == nullptr)
        {
            SetTokenClient();
            LOGINFO("OttServices: TokenClient initialized inside GetAppCIMAToken");
        }

        const std::string cacheKey = std::string("platform:") + appId;

        // Attempt cache hit
        if (_tokenCache.Get(cacheKey, token)) {
            LOGINFO("OttServices: GetAppCIMAToken cache hit (appId='%s')", appId.c_str());
            return Core::ERROR_NONE;
        }

        // Resolve SAT and xACT internally
        std::string sat;
        std::string xact;
        uint64_t satExpiry = 0;
        uint64_t xactExpiry = 0;

        if (!FetchSat(sat, satExpiry)) {
            LOGERR("OttServices: FetchSat failed");
            return Core::ERROR_UNAVAILABLE;
        }
        if (!FetchXact(appId, xact, xactExpiry)) {
            LOGERR("OttServices: FetchXact failed");
            return Core::ERROR_UNAVAILABLE;
        }

        std::lock_guard<std::mutex> guard(_tokenMutex);
        if (!_token) {
            _token = std::make_shared<TokenClient>(_tokenEndpoint, _tokenUseTls);
        }

        std::string err;
        uint32_t expiresInSec = 0;
        const bool ok = _token->GetPlatformToken(appId, xact, sat, token, expiresInSec, err);
        if (!ok) {
            LOGERR("OttServices: GetAppCIMAToken failed: %s", err.c_str());
            token.clear();
            return Core::ERROR_UNAVAILABLE;
        }

        // Cache with conservative expiry (earliest of sat/xact/token)
        const uint64_t now = NowEpochSec();
        uint64_t tokenExpiry = (expiresInSec > 0 ? now + static_cast<uint64_t>(expiresInSec) : 0);
        uint64_t expiry = tokenExpiry;
        if (expiry == 0 || (satExpiry > 0 && (satExpiry < expiry || expiry == 0))) expiry = satExpiry;
        if (expiry == 0 || (xactExpiry > 0 && (xactExpiry < expiry || expiry == 0))) expiry = xactExpiry;

        if (expiry > now) {
            TokenEntry entry;
            entry.tokenJson = token;
            entry.expiryEpochSec = expiry;
            _tokenCache.Put(cacheKey, entry);
        }

        LOGINFO("OttServices: GetAppCIMAToken success (token=[REDACTED])");
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
