#include "OttServicesImplementation.h"

#include <core/Enumerate.h>
#include "UtilsLogging.h"

// Use in-module permission cache and JSONRPC direct link utils
#include "OttPermissionCache.h"
#include "UtilsJsonrpcDirectLink.h"

// Token client and cache
#include "TokenClient.h"
#include "TokenCache.h"

#include <chrono>
#include <cstdlib>

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(OttServicesImplementation, 1, 0);

    namespace {
        static constexpr const char* kDefaultPermsEndpoint = "thor-permission.svc.thor.comcast.com:443";
        static constexpr const char* kDefaultTokenEndpoint = kDefaultPermsEndpoint; // fallback if token endpoint not configured
        static constexpr const char* kAuthServiceCallsign = "org.rdk.AuthService";
    }

    OttServicesImplementation::OttServicesImplementation()
        : _service(nullptr)
        , _state(_T("uninitialized"))
        , _perms(nullptr)
        , _permsEndpoint(kDefaultPermsEndpoint)
        , _permsUseTls(true)
        , _permsMutex()
        , _permsClientUseTls(true)
        , _token(nullptr)
        , _tokenEndpoint(kDefaultTokenEndpoint)
        , _tokenUseTls(true)
        , _tokenMutex()
        , _tokenClientUseTls(true)
        , _refCount(1) {
    }

    OttServicesImplementation::~OttServicesImplementation() = default;

    uint32_t OttServicesImplementation::Configure(PluginHost::IShell* shell) {

        LOGINFO("Configuring OttServices");
        ASSERT(shell != nullptr);
        shell->AddRef();
        // Configure using the provided shell (mirrors Initialize logic; return Core::ERROR_NONE on success)
        Initialize(shell);
        return Core::ERROR_NONE;
    }

    string OttServicesImplementation::Initialize(PluginHost::IShell* service) {
        _service = service;
        _state = _T("ready");

        // Load configuration: PermissionsEndpoint, UseTls; TokenEndpoint (optional), UseTlsToken (optional)
        if (_service != nullptr) {
            Config cfg;
            cfg.FromString(_service->ConfigLine());
            _permsEndpoint = (cfg.PermissionsEndpoint.IsSet() && !cfg.PermissionsEndpoint.Value().empty())
                                ? cfg.PermissionsEndpoint.Value()
                                : std::string(kDefaultPermsEndpoint);
            _permsUseTls = cfg.UseTls.IsSet() ? static_cast<bool>(cfg.UseTls.Value()) : true;

            _tokenEndpoint = (cfg.TokenEndpoint.IsSet() && !cfg.TokenEndpoint.Value().empty())
                                ? cfg.TokenEndpoint.Value()
                                : _permsEndpoint;
            _tokenUseTls = cfg.UseTlsToken.IsSet() ? static_cast<bool>(cfg.UseTlsToken.Value()) : _permsUseTls;
        }

        LOGINFO("OttServices: Initialize implementation (permsEndpoint=%s, permsTls=%s, tokenEndpoint=%s, tokenTls=%s)",
                _permsEndpoint.c_str(), _permsUseTls ? "true" : "false",
                _tokenEndpoint.c_str(), _tokenUseTls ? "true" : "false");

        // Reset clients on (re)configure; will be lazily created on first use.
        {
            std::lock_guard<std::mutex> lockPerms(_permsMutex);
            _perms.reset();
        }
        {
            std::lock_guard<std::mutex> lockToken(_tokenMutex);
            _token.reset();
        }

        // Warm up cache file load
        OttPermissionCache::Instance();

        // Return empty string on success per Thunder conventions.
        return string();
    }

    void OttServicesImplementation::Deinitialize(PluginHost::IShell* /*service*/) {
        {
            std::lock_guard<std::mutex> lockPerms(_permsMutex);
            _perms.reset();
        }
        {
            std::lock_guard<std::mutex> lockToken(_tokenMutex);
            _token.reset();
        }
        _state = _T("deinitialized");
        _service = nullptr;
        LOGINFO("OttServices: Deinitialize implementation");
    }


    // Exchange::IOttServices implementation and local variants

    Core::hresult OttServicesImplementation::Ping(const string& message, string& reply) {
        LOGINFO("OttServices: Ping called");
        reply = (message.empty() ? string(_T("pong")) : (string(_T("pong: ")) + message));
        LOGINFO("OttServices: Ping called (message='%s', reply='%s')", message.c_str(), reply.c_str());
        return Core::ERROR_NONE;
    }

    Core::hresult OttServicesImplementation::GetPermissions(const string& appId, std::vector<string>& permissionsOut) {
        LOGINFO("OttServices: GetPermissions called");
        permissionsOut = OttPermissionCache::Instance().GetPermissions(appId);
        LOGINFO("OttServices: GetPermissions (vector) appId='%s', count=%zu", appId.c_str(), permissionsOut.size());
        return Core::ERROR_NONE;
    }

    Core::hresult OttServicesImplementation::GetPermissions(const string& appId, string& permissionsJson) {
        LOGINFO("OttServices GetPermissions 2 called");
        std::vector<string> permissions;
        Core::hresult rc = GetPermissions(appId, permissions);
        if (rc != Core::ERROR_NONE) {
            LOGWARN("OttServices: GetPermissions (json) failed early (appId='%s', rc=%u)", appId.c_str(), rc);
            return rc;
        }

        // Serialize to JSON array string using Core::JSON classes for correctness.
        Core::JSON::ArrayType<Core::JSON::String> array;
        for (const auto& p : permissions) {
            Core::JSON::String v; v = p;
            array.Add(v);
        }
        permissionsJson.clear();
        array.ToString(permissionsJson);
        LOGINFO("OttServices: GetPermissions (json) appId='%s', json='%s'", appId.c_str(), permissionsJson.c_str());
        return Core::ERROR_NONE;
    }

    Core::hresult OttServicesImplementation::InvalidatePermissions(const string& appId) {
        OttPermissionCache::Instance().Invalidate(appId);
        LOGINFO("OttServices: InvalidatePermissions appId='%s'", appId.c_str());
        return Core::ERROR_NONE;
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
        if (_service == nullptr) {
            LOGERR("OttServices: CollectAuthMetadata called with null service");
            return false;
        }

        auto link = Utils::GetThunderControllerClient(_service, kAuthServiceCallsign);
        if (link == nullptr) {
            LOGERR("OttServices: failed to acquire AuthService direct link");
            return false;
        }

        // Use JsonObject for request/response parity with DeviceInfo.cpp
        JsonObject params;
        JsonObject response;

        // Service Access Token (require token and expires)
        uint32_t result = link->Invoke<JsonObject, JsonObject>("getServiceAccessToken", params, response);
        if ((result == Core::ERROR_NONE) && response.HasLabel("token") && response.HasLabel("expires")) {
            bearerToken = response["token"].String();
        } else {
            LOGERR("OttServices: getServiceAccessToken failed rc=%u", result);
            return false;
        }
        response.Clear();

        // XDeviceId
        result = link->Invoke<JsonObject, JsonObject>("getXDeviceId", params, response);
        if ((result == Core::ERROR_NONE) && response.HasLabel("xDeviceId")) {
            deviceId = response["xDeviceId"].String();
            if (deviceId.empty()) {
                LOGERR("OttServices: getXDeviceId returned empty xDeviceId");
                return false;
            }
        } else {
            LOGERR("OttServices: getXDeviceId failed rc=%u", result);
            return false;
        }
        response.Clear();

        // ServiceAccountId
        result = link->Invoke<JsonObject, JsonObject>("getServiceAccountId", params, response);
        if ((result == Core::ERROR_NONE) && response.HasLabel("serviceAccountId")) {
            accountId = response["serviceAccountId"].String();
            if (accountId.empty()) {
                LOGERR("OttServices: getServiceAccountId returned empty serviceAccountId");
                return false;
            }
        } else {
            LOGERR("OttServices: getServiceAccountId failed rc=%u", result);
            return false;
        }
        response.Clear();

        // PartnerId (from getDeviceId payload)
        result = link->Invoke<JsonObject, JsonObject>("getDeviceId", params, response);
        if ((result == Core::ERROR_NONE) && response.HasLabel("partnerId")) {
            partnerId = response["partnerId"].String();
            if (partnerId.empty()) {
                LOGERR("OttServices: getDeviceId returned empty partnerId");
                return false;
            }
        } else {
            LOGERR("OttServices: getDeviceId failed rc=%u", result);
            return false;
        }

        // Final sanity check
        if (bearerToken.empty() || deviceId.empty() || accountId.empty() || partnerId.empty()) {
            LOGERR("OttServices: one or more required AuthService values are empty (token=[REDACTED], device=%s, account=%s, partner=%s)",
                   deviceId.c_str(), accountId.c_str(), partnerId.c_str());
            return false;
        }

        return true;
    }

    Core::hresult OttServicesImplementation::UpdatePermissionsCache(const string& appId, uint32_t& updatedCount) {
        updatedCount = 0;

        if (appId.empty()) {
            LOGERR("OttServices: UpdatePermissionsCache bad request (empty appId)");
            return Core::ERROR_BAD_REQUEST;
        }

        // Ensure (lazily) the permissions client is created and up-to-date with current configuration.
        EnsurePerms();

        LOGINFO("OttServices: UpdatePermissionsCache start (appId='%s', endpoint=%s)", appId.c_str(), _perms->Endpoint().c_str());

        std::string token;
        std::string deviceId;
        std::string accountId;
        std::string partnerId;
        if (!CollectAuthMetadata(token, deviceId, accountId, partnerId)) {
            LOGERR("OttServices: UpdatePermissionsCache auth metadata collection failed (appId='%s')", appId.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        std::vector<std::string> filters; // none for now
        std::vector<std::string> fetched;
        const uint32_t rc = _perms->EnumeratePermissions(appId, partnerId, token, deviceId, accountId, filters, fetched);
        if (rc != Core::ERROR_NONE) {
            LOGWARN("OttServices: UpdatePermissionsCache fetch failed (appId='%s', rc=%u)", appId.c_str(), rc);
            return rc;
        }

        OttPermissionCache::Instance().UpdateCache(appId, fetched);
        updatedCount = static_cast<uint32_t>(fetched.size());
        LOGINFO("OttServices: UpdatePermissionsCache updated (appId='%s', count=%u)", appId.c_str(), updatedCount);
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

        auto link = Utils::GetThunderControllerClient(_service, kAuthServiceCallsign);
        if (link == nullptr) {
            LOGERR("OttServices: Failed to acquire AuthService direct link for SAT");
            return false;
        }

        JsonObject params;
        JsonObject response;
        uint32_t rc = link->Invoke<JsonObject, JsonObject>("getServiceAccessToken", params, response);
        if (rc != Core::ERROR_NONE || !response.HasLabel("token")) {
            LOGERR("OttServices: getServiceAccessToken failed rc=%u", rc);
            return false;
        }
        sat = response["token"].String();

        // Optional expiry parsing
        if (response.HasLabel("expires")) {
            const string expStr = response["expires"].String();
            if (!expStr.empty()) {
                char* endPtr = nullptr;
                uint64_t v = std::strtoull(expStr.c_str(), &endPtr, 10);
                if (endPtr != expStr.c_str() && v > 0) {
                    expiryEpoch = v;
                }
            }
        }
        return !sat.empty();
    }

    bool OttServicesImplementation::FetchXact(const std::string& appId, std::string& xact, uint64_t& expiryEpoch) const {
        xact.clear();
        expiryEpoch = 0;

        if (_service == nullptr) {
            LOGERR("OttServices: FetchXact called with null service");
            return false;
        }

        auto link = Utils::GetThunderControllerClient(_service, kAuthServiceCallsign);
        if (link == nullptr) {
            LOGERR("OttServices: Failed to acquire AuthService direct link for xACT");
            return false;
        }

        JsonObject params;
        if (!appId.empty()) {
            params["appId"] = appId;
        }

        JsonObject response;
        uint32_t rc = link->Invoke<JsonObject, JsonObject>("getAuthToken", params, response);
        if (rc != Core::ERROR_NONE) {
            LOGERR("OttServices: getAuthToken failed rc=%u", rc);
            return false;
        }

        // Accept several possible field names
        if (response.HasLabel("token")) {
            xact = response["token"].String();
        } else if (response.HasLabel("xact")) {
            xact = response["xact"].String();
        } else if (response.HasLabel("xACT")) {
            xact = response["xACT"].String();
        } else if (response.HasLabel("access_token")) {
            xact = response["access_token"].String();
        }

        // Optional expiry parsing
        if (response.HasLabel("expires_in")) {
            const string expInStr = response["expires_in"].String();
            if (!expInStr.empty()) {
                char* endPtr = nullptr;
                uint64_t v = std::strtoull(expInStr.c_str(), &endPtr, 10);
                if (endPtr != expInStr.c_str() && v > 0) {
                    expiryEpoch = NowEpochSec() + v;
                }
            }
        } else if (response.HasLabel("expires")) {
            const string expStr = response["expires"].String();
            if (!expStr.empty()) {
                char* endPtr = nullptr;
                uint64_t v = std::strtoull(expStr.c_str(), &endPtr, 10);
                if (endPtr != expStr.c_str() && v > 0) {
                    expiryEpoch = v;
                }
            }
        }

        if (xact.empty()) {
            LOGERR("OttServices: getAuthToken returned empty xACT");
            return false;
        }
        return true;
    }

    Core::hresult OttServicesImplementation::GetDistributorToken(const string& appId,
                                                                 string& token)
    {
        LOGINFO("OttServices: GetDistributorToken called (appId='%s')", appId.c_str());
        if (appId.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        const std::string cacheKey = std::string("platform:") + appId;

        // Attempt cache hit
        if (_tokenCache.Get(cacheKey, token)) {
            LOGINFO("OttServices: GetDistributorToken cache hit (appId='%s')", appId.c_str());
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

        // Ensure (lazily) the token client is created and up-to-date with current configuration.
        EnsureToken();

        std::string err;
        uint32_t expiresInSec = 0;
        const bool ok = _token->GetPlatformToken(appId, xact, sat, token, expiresInSec, err);
        if (!ok) {
            LOGERR("OttServices: GetDistributorToken failed: %s", err.c_str());
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
            TokenEntry entry{ token, expiry };
            _tokenCache.Put(cacheKey, entry);
        }

        LOGINFO("OttServices: GetDistributorToken success (token=[REDACTED])");
        return Core::ERROR_NONE;
    }

    Core::hresult OttServicesImplementation::GetAuthToken(const string& appId,
                                                          string& token)
    {
        LOGINFO("OttServices: GetAuthToken called (appId='%s')", appId.c_str());
        if (appId.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }

        const std::string cacheKey = std::string("auth:") + appId;

        // Attempt cache hit
        if (_tokenCache.Get(cacheKey, token)) {
            LOGINFO("OttServices: GetAuthToken cache hit (appId='%s')", appId.c_str());
            return Core::ERROR_NONE;
        }

        // Resolve SAT internally
        std::string sat;
        uint64_t satExpiry = 0;
        if (!FetchSat(sat, satExpiry)) {
            LOGERR("OttServices: FetchSat failed");
            return Core::ERROR_UNAVAILABLE;
        }

        // Ensure (lazily) the token client is created and up-to-date with current configuration.
        EnsureToken();

        std::string err;
        uint32_t expiresInSec = 0;
        const bool ok = _token->GetAuthToken(appId, sat, token, expiresInSec, err);
        if (!ok) {
            LOGERR("OttServices: GetAuthToken failed: %s", err.c_str());
            token.clear();
            return Core::ERROR_UNAVAILABLE;
        }

        // Cache using expires_in if present, bounded by SAT expiry
        const uint64_t now = NowEpochSec();
        uint64_t expiry = (expiresInSec > 0 ? now + static_cast<uint64_t>(expiresInSec) : 0);
        if (expiry == 0 || (satExpiry > 0 && (satExpiry < expiry || expiry == 0))) {
            expiry = satExpiry;
        }
        if (expiry > now) {
            TokenEntry entry{ token, expiry };
            _tokenCache.Put(cacheKey, entry);
        }

        LOGINFO("OttServices: GetAuthToken success (token=[REDACTED])");
        return Core::ERROR_NONE;
    }

    // ---- Lazy client helpers ----

    void OttServicesImplementation::EnsurePerms() {
        const std::string desiredEndpoint = _permsEndpoint.empty() ? std::string(kDefaultPermsEndpoint) : _permsEndpoint;
        const bool desiredTls = _permsUseTls;

        std::lock_guard<std::mutex> lock(_permsMutex);

        bool needCreate = !_perms;
        if (!needCreate) {
            if (_perms->Endpoint() != desiredEndpoint || _permsClientUseTls != desiredTls) {
                needCreate = true;
            }
        }

        if (needCreate) {
            LOGINFO("OttServices: (Re)creating PermissionsClient (endpoint=%s, tls=%s)",
                    desiredEndpoint.c_str(), desiredTls ? "true" : "false");
            _perms.reset(new PermissionsClient(desiredEndpoint, desiredTls));
            _permsClientUseTls = desiredTls;
        }
    }

    void OttServicesImplementation::EnsureToken() {
        // token endpoint falls back to permissions endpoint if not explicitly set
        const std::string effectiveEndpoint = (!_tokenEndpoint.empty() ? _tokenEndpoint :
                                               (!_permsEndpoint.empty() ? _permsEndpoint : std::string(kDefaultTokenEndpoint)));
        const bool desiredTls = _tokenUseTls;

        std::lock_guard<std::mutex> lock(_tokenMutex);

        bool needCreate = !_token;
        if (!needCreate) {
            if (_token->Endpoint() != effectiveEndpoint || _tokenClientUseTls != desiredTls) {
                needCreate = true;
            }
        }

        if (needCreate) {
            LOGINFO("OttServices: (Re)creating TokenClient (endpoint=%s, tls=%s)",
                    effectiveEndpoint.c_str(), desiredTls ? "true" : "false");
            _token.reset(new TokenClient(effectiveEndpoint, desiredTls));
            _tokenClientUseTls = desiredTls;
        }
    }

} // namespace Plugin
} // namespace WPEFramework
