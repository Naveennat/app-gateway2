#include "OttServicesImplementation.h"

#include <core/Enumerate.h>
#include "UtilsLogging.h"

// Use shared permission cache and JSONRPC direct link utils from Supporting_Files
#include "OttPermissionsCache.h"
#include "UtilsJsonrpcDirectLink.h"

namespace WPEFramework {
namespace Plugin {

    namespace {
        static constexpr const char* kDefaultPermsEndpoint = "thor-permission.svc.thor.comcast.com:443";
        static constexpr const char* kAuthServiceCallsign = "org.rdk.AuthService";
    }

    OttServicesImplementation::OttServicesImplementation()
        : _service(nullptr)
        , _state(_T("uninitialized"))
        , _perms(nullptr)
        , _permsEndpoint(kDefaultPermsEndpoint)
        , _permsUseTls(true) {
    }

    OttServicesImplementation::~OttServicesImplementation() = default;

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
    }

    uint32_t OttServicesImplementation::Ping(const string& message, string& reply) {
        // Simple echo behavior; actual behavior should follow spec if different.
        reply = (message.empty() ? string(_T("pong")) : (string(_T("pong: ")) + message));
        return Core::ERROR_NONE;
    }

    uint32_t OttServicesImplementation::GetPermissions(const string& appId, std::vector<string>& permissionsOut) {
        permissionsOut = OttPermissionCache::Instance().GetPermissions(appId);
        return Core::ERROR_NONE;
    }

    uint32_t OttServicesImplementation::InvalidatePermissions(const string& appId) {
        OttPermissionCache::Instance().Invalidate(appId);
        return Core::ERROR_NONE;
    }

    bool OttServicesImplementation::CollectAuthMetadata(std::string& bearerToken,
                                                        std::string& deviceId,
                                                        std::string& accountId,
                                                        std::string& partnerId) const {
        if (_service == nullptr) {
            return false;
        }
        auto link = Utils::GetThunderControllerClient(_service, kAuthServiceCallsign);
        if (!link) {
            LOGERR("OttServices: failed to acquire AuthService direct link");
            return false;
        }

        WPEFramework::Core::JSON::VariantContainer params;
        WPEFramework::Core::JSON::VariantContainer response;

        // Service Access Token
        uint32_t rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getServiceAccessToken", params, response);
        if ((rc != Core::ERROR_NONE) || (response.HasLabel(_T("token")) == false)) {
            LOGERR("OttServices: getServiceAccessToken failed rc=%u", rc);
            return false;
        }
        bearerToken = response[_T("token")].String();
        response.Clear();

        // XDeviceId
        rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getXDeviceId", params, response);
        if ((rc != Core::ERROR_NONE) || (response.HasLabel(_T("xDeviceId")) == false)) {
            LOGERR("OttServices: getXDeviceId failed rc=%u", rc);
            return false;
        }
        deviceId = response[_T("xDeviceId")].String();
        response.Clear();

        // ServiceAccountId
        rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getServiceAccountId", params, response);
        if ((rc != Core::ERROR_NONE) || (response.HasLabel(_T("serviceAccountId")) == false)) {
            LOGERR("OttServices: getServiceAccountId failed rc=%u", rc);
            return false;
        }
        accountId = response[_T("serviceAccountId")].String();
        response.Clear();

        // PartnerId (from getDeviceId payload)
        rc = link->Invoke<WPEFramework::Core::JSON::VariantContainer, WPEFramework::Core::JSON::VariantContainer>("getDeviceId", params, response);
        if ((rc != Core::ERROR_NONE) || (response.HasLabel(_T("partnerId")) == false)) {
            LOGERR("OttServices: getDeviceId failed rc=%u", rc);
            return false;
        }
        partnerId = response[_T("partnerId")].String();

        // Minimal validation
        if (bearerToken.empty() || deviceId.empty() || accountId.empty() || partnerId.empty()) {
            LOGERR("OttServices: one or more required AuthService values are empty (token=[REDACTED], device=%s, account=%s, partner=%s)",
                   deviceId.c_str(), accountId.c_str(), partnerId.c_str());
            return false;
        }

        return true;
    }

    uint32_t OttServicesImplementation::UpdatePermissionsCache(const string& appId, uint32_t& updatedCount) {
        updatedCount = 0;

        if (appId.empty()) {
            return Core::ERROR_BAD_REQUEST;
        }
        if (!_perms) {
            _perms.reset(new PermissionsClient(_permsEndpoint.empty() ? kDefaultPermsEndpoint : _permsEndpoint, _permsUseTls));
        }

        std::string token;
        std::string deviceId;
        std::string accountId;
        std::string partnerId;
        if (!CollectAuthMetadata(token, deviceId, accountId, partnerId)) {
            return Core::ERROR_UNAVAILABLE;
        }

        std::vector<std::string> filters; // none for now
        std::vector<std::string> fetched;
        const uint32_t rc = _perms->EnumeratePermissions(appId, partnerId, token, deviceId, accountId, filters, fetched);
        if (rc != Core::ERROR_NONE) {
            return rc;
        }

        OttPermissionCache::Instance().UpdateCache(appId, fetched);
        updatedCount = static_cast<uint32_t>(fetched.size());
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
