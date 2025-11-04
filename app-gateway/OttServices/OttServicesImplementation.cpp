#include "OttServicesImplementation.h"

#include <core/Enumerate.h>
#include "UtilsLogging.h"

 // Use in-module permission cache and JSONRPC direct link utils
#include "OttPermissionCache.h"
#include "UtilsJsonrpcDirectLink.h"

namespace WPEFramework {
namespace Plugin {

    SERVICE_REGISTRATION(OttServicesImplementation, 1, 0);

    namespace {
        static constexpr const char* kDefaultPermsEndpoint = "thor-permission.svc.thor.comcast.com:443";
        static constexpr const char* kAuthServiceCallsign = "org.rdk.AuthService";
    }

    OttServicesImplementation::OttServicesImplementation()
        : _service(nullptr)
        , _state(_T("uninitialized"))
        , _perms(nullptr)
        , _permsEndpoint(kDefaultPermsEndpoint)
        , _permsUseTls(true)
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

        // Load configuration: PermissionsEndpoint, UseTls
        if (_service != nullptr) {
            Config cfg;
            cfg.FromString(_service->ConfigLine());
            _permsEndpoint = (cfg.PermissionsEndpoint.IsSet() && !cfg.PermissionsEndpoint.Value().empty())
                                ? cfg.PermissionsEndpoint.Value()
                                : std::string(kDefaultPermsEndpoint);
            _permsUseTls = cfg.UseTls.IsSet() ? static_cast<bool>(cfg.UseTls.Value()) : true;
        }

        LOGINFO("OttServices: Initialize implementation (endpoint=%s, tls=%s)",
                _permsEndpoint.c_str(), _permsUseTls ? "true" : "false");

        // Create permissions client (works even if gRPC disabled; will return ERROR_UNAVAILABLE on use)
        _perms.reset(new PermissionsClient(_permsEndpoint, _permsUseTls));

        // Warm up cache file load
        OttPermissionCache::Instance();

        // Return empty string on success per Thunder conventions.
        return string();
    }

    void OttServicesImplementation::Deinitialize(PluginHost::IShell* /*service*/) {
        _perms.reset();
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
        if (!_perms) {
            _perms.reset(new PermissionsClient(_permsEndpoint.empty() ? kDefaultPermsEndpoint : _permsEndpoint, _permsUseTls));
        }

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

} // namespace Plugin
} // namespace WPEFramework