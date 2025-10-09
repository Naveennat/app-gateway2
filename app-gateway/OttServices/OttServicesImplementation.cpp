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

    // IUnknown implementation
    void OttServicesImplementation::AddRef() const {
        // Align with WPEFramework::Core::IReferenceCounted::AddRef() const returning void
        // Just increment the ref count; caller does not expect a return value.
        _refCount.fetch_add(1, std::memory_order_relaxed);
    }

    uint32_t OttServicesImplementation::Release() const {
        const uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0) {
            delete this;
        }
        return count;
    }

    void* OttServicesImplementation::QueryInterface(const uint32_t id) {
        void* result = nullptr;
        if (id == Exchange::IOttServices::ID) {
            result = static_cast<Exchange::IOttServices*>(this);
        } else if (id == Core::IUnknown::ID) {
            result = static_cast<Core::IUnknown*>(this);
        }
        if (result != nullptr) {
            AddRef();
        }
        return result;
    }

    // Exchange::IOttServices implementation and local variants

    Core::hresult OttServicesImplementation::Ping(const string& message, string& reply) {
        reply = (message.empty() ? string(_T("pong")) : (string(_T("pong: ")) + message));
        LOGINFO("OttServices: Ping called (message='%s', reply='%s')", message.c_str(), reply.c_str());
        return Core::ERROR_NONE;
    }

    Core::hresult OttServicesImplementation::GetPermissions(const string& appId, std::vector<string>& permissionsOut) {
        permissionsOut = OttPermissionCache::Instance().GetPermissions(appId);
        LOGINFO("OttServices: GetPermissions (vector) appId='%s', count=%zu", appId.c_str(), permissionsOut.size());
        return Core::ERROR_NONE;
    }

    Core::hresult OttServicesImplementation::GetPermissions(const string& appId, string& permissionsJson) {
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
