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
 
#include "PermissionsClient.h"

#include <algorithm>
#include <utility>
#include <grpcpp/security/credentials.h>

// Lightweight logging; avoid leaking secrets
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

    namespace {
        inline void SetCommonMetadata(grpc::ClientContext& ctx,
                                      const std::string& bearerToken,
                                      const std::string& deviceId,
                                      const std::string& accountId,
                                      const std::string& partnerId) {
            if (!bearerToken.empty()) {
                ctx.AddMetadata("authorization", std::string("Bearer ") + bearerToken);
            }
            if (!deviceId.empty()) {
                ctx.AddMetadata("deviceid", deviceId);
            }
            if (!accountId.empty()) {
                ctx.AddMetadata("accountid", accountId);
            }
            if (!partnerId.empty()) {
                ctx.AddMetadata("partnerid", partnerId);
            }
        }

        inline uint32_t MapGrpcStatusToCore(const grpc::Status& status) {
            if (status.ok()) {
                return Core::ERROR_NONE;
            }
            switch (status.error_code()) {
                case grpc::StatusCode::UNAUTHENTICATED:
                case grpc::StatusCode::PERMISSION_DENIED:
                    return Core::ERROR_PRIVILIGED_REQUEST;
                case grpc::StatusCode::UNAVAILABLE:
                    return Core::ERROR_UNAVAILABLE;
                case grpc::StatusCode::DEADLINE_EXCEEDED:
                    return Core::ERROR_TIMEDOUT;
                case grpc::StatusCode::INVALID_ARGUMENT:
                case grpc::StatusCode::FAILED_PRECONDITION:
                    return Core::ERROR_BAD_REQUEST;
                default:
                    return Core::ERROR_GENERAL;
            }
        }
    } // namespace

    PermissionsClient::PermissionsClient(const std::string& endpoint, bool useTls)
        : _endpoint(endpoint)
        , _channel(CreateChannel(endpoint, useTls))
        , _stub(ottx::permission::AppPermissionsService::NewStub(_channel))
    {
    }

    PermissionsClient::PermissionsClient(const std::string& endpoint, std::shared_ptr<grpc::ChannelCredentials> creds)
        : _endpoint(endpoint)
        , _channel(CreateChannel(endpoint, creds))
        , _stub(ottx::permission::AppPermissionsService::NewStub(_channel))
    {
    }

    PermissionsClient::~PermissionsClient() = default;

    std::string PermissionsClient::Endpoint() const {
        return _endpoint;
    }

    std::shared_ptr<grpc::Channel> PermissionsClient::CreateChannel(const std::string& endpoint, bool useTls) {
        if (useTls) {
            grpc::SslCredentialsOptions ssl_opts;
            auto creds = grpc::SslCredentials(ssl_opts);
            return grpc::CreateChannel(endpoint, creds);
        } else {
            return grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
        }
    }

    std::shared_ptr<grpc::Channel> PermissionsClient::CreateChannel(
        const std::string& endpoint, const std::shared_ptr<grpc::ChannelCredentials>& creds) {
        return grpc::CreateChannel(endpoint, creds);
    }

    uint32_t PermissionsClient::EnumeratePermissions(const std::string& appId,
                                                     const std::string& partnerId,
                                                     const std::string& bearerToken,
                                                     const std::string& deviceId,
                                                     const std::string& accountId,
                                                     const std::vector<std::string>& filters,
                                                     std::vector<std::string>& outPermissions) const
    {
        outPermissions.clear();

        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        // Build request
        ottx::permission::EnumeratePermissionsRequest request;
        auto* key = request.mutable_app_key();
        key->set_app(appId);
        key->set_syndication_partner_id(partnerId);

        for (const auto& f : filters) {
            if (!f.empty()) {
                request.add_permission_filters(f);
            }
        }

        // Build context with required metadata. Redact secrets in logs!
        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::EnumeratePermissionsResponse response;
        grpc::Status status = _stub->EnumeratePermissions(&ctx, request, &response);

        if (!status.ok()) {
            uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: EnumeratePermissions failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), appId.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outPermissions.reserve(static_cast<size_t>(response.permissions_size()));
        for (const auto& p : response.permissions()) {
            outPermissions.emplace_back(p);
        }

        LOGINFO("PermissionsClient: EnumeratePermissions ok (endpoint=%s, app=%s, count=%zu)",
                _endpoint.c_str(), appId.c_str(), outPermissions.size());

        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::Get(const std::string& appId,
                                    const std::string& partnerId,
                                    const std::string& bearerToken,
                                    const std::string& deviceId,
                                    const std::string& accountId,
                                    ottx::permission::ApplicationPermissions& outAppPermissions) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::GetRequest request;
        auto* key = request.mutable_app_key();
        key->set_app(appId);
        key->set_syndication_partner_id(partnerId);

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::GetResponse response;
        grpc::Status status = _stub->Get(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: Get failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), appId.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outAppPermissions = response.app_permissions();
        LOGINFO("PermissionsClient: Get ok (endpoint=%s, app=%s, bundles=%d, includes=%d, excludes=%d)",
                _endpoint.c_str(), appId.c_str(),
                outAppPermissions.bundles_size(),
                outAppPermissions.includes_size(),
                outAppPermissions.excludes_size());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::Grant(const std::string& appId,
                                      const std::string& partnerId,
                                      const std::vector<std::string>& bundles,
                                      const std::vector<std::string>& permissions,
                                      const std::string& bearerToken,
                                      const std::string& deviceId,
                                      const std::string& accountId,
                                      ottx::permission::ApplicationPermissions& outAppPermissions) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::GrantRequest request;
        auto* key = request.mutable_app_key();
        key->set_app(appId);
        key->set_syndication_partner_id(partnerId);

        for (const auto& b : bundles) {
            if (!b.empty()) request.add_bundles(b);
        }
        for (const auto& p : permissions) {
            if (!p.empty()) request.add_permissions(p);
        }

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::GrantResponse response;
        grpc::Status status = _stub->Grant(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: Grant failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), appId.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outAppPermissions = response.app_permissions();
        LOGINFO("PermissionsClient: Grant ok (endpoint=%s, app=%s)", _endpoint.c_str(), appId.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::Set(const std::string& appId,
                                    const std::string& partnerId,
                                    const std::vector<std::string>& bundles,
                                    const std::vector<std::string>& includes,
                                    const std::vector<std::string>& excludes,
                                    const std::string& bearerToken,
                                    const std::string& deviceId,
                                    const std::string& accountId,
                                    ottx::permission::ApplicationPermissions& outAppPermissions) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::SetRequest request;
        auto* ap = request.mutable_app_permissions();
        auto* key = ap->mutable_app_key();
        key->set_app(appId);
        key->set_syndication_partner_id(partnerId);

        for (const auto& b : bundles) {
            if (!b.empty()) ap->add_bundles(b);
        }
        for (const auto& i : includes) {
            if (!i.empty()) ap->add_includes(i);
        }
        for (const auto& e : excludes) {
            if (!e.empty()) ap->add_excludes(e);
        }

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::SetResponse response;
        grpc::Status status = _stub->Set(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: Set failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), appId.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outAppPermissions = response.app_permissions();
        LOGINFO("PermissionsClient: Set ok (endpoint=%s, app=%s)", _endpoint.c_str(), appId.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::Revoke(const std::string& appId,
                                       const std::string& partnerId,
                                       const std::vector<std::string>& bundles,
                                       const std::vector<std::string>& permissions,
                                       const std::string& bearerToken,
                                       const std::string& deviceId,
                                       const std::string& accountId,
                                       ottx::permission::ApplicationPermissions& outAppPermissions) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::RevokeRequest request;
        auto* key = request.mutable_app_key();
        key->set_app(appId);
        key->set_syndication_partner_id(partnerId);

        for (const auto& b : bundles) {
            if (!b.empty()) request.add_bundles(b);
        }
        for (const auto& p : permissions) {
            if (!p.empty()) request.add_permissions(p);
        }

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::RevokeResponse response;
        grpc::Status status = _stub->Revoke(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: Revoke failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), appId.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outAppPermissions = response.app_permissions();
        LOGINFO("PermissionsClient: Revoke ok (endpoint=%s, app=%s)", _endpoint.c_str(), appId.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::Delete(const std::string& appId,
                                       const std::string& partnerId,
                                       const std::string& bearerToken,
                                       const std::string& deviceId,
                                       const std::string& accountId) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::DeleteRequest request;
        auto* key = request.mutable_app_key();
        key->set_app(appId);
        key->set_syndication_partner_id(partnerId);

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::DeleteResponse response;
        grpc::Status status = _stub->Delete(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: Delete failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), appId.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        LOGINFO("PermissionsClient: Delete ok (endpoint=%s, app=%s)", _endpoint.c_str(), appId.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::Authorize(const std::string& sessionToken,
                                          const std::vector<std::string>& filters,
                                          const std::string& bearerToken,
                                          const std::string& deviceId,
                                          const std::string& accountId,
                                          const std::string& partnerId,
                                          std::vector<std::string>& outPermissions) const
    {
        outPermissions.clear();

        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::AuthorizeRequest request;
        request.set_session_token(sessionToken);
        for (const auto& f : filters) {
            if (!f.empty()) request.add_permission_filters(f);
        }

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::AuthorizeResponse response;
        grpc::Status status = _stub->Authorize(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: Authorize failed (endpoint=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        const auto& auth = response.authorization();
        outPermissions.reserve(static_cast<size_t>(auth.permissions_size()));
        for (const auto& p : auth.permissions()) {
            outPermissions.emplace_back(p);
        }

        LOGINFO("PermissionsClient: Authorize ok (endpoint=%s, count=%zu)",
                _endpoint.c_str(), outPermissions.size());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::GetAllForApp(const std::string& app,
                                             const std::string& bearerToken,
                                             const std::string& deviceId,
                                             const std::string& accountId,
                                             const std::string& partnerId,
                                             std::vector<ottx::permission::ApplicationPermissions>& outList) const
    {
        outList.clear();

        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::GetAllForAppRequest request;
        request.set_app(app);

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::GetAllForAppResponse response;
        grpc::Status status = _stub->GetAllForApp(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: GetAllForApp failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), app.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outList.reserve(static_cast<size_t>(response.app_permissions_size()));
        for (const auto& ap : response.app_permissions()) {
            outList.emplace_back(ap);
        }

        LOGINFO("PermissionsClient: GetAllForApp ok (endpoint=%s, app=%s, count=%zu)",
                _endpoint.c_str(), app.c_str(), outList.size());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::GetAllAppKeys(const std::string& bearerToken,
                                              const std::string& deviceId,
                                              const std::string& accountId,
                                              const std::string& partnerId,
                                              std::vector<ottx::permission::AppKeys>& outKeys) const
    {
        outKeys.clear();

        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::GetAllAppKeysRequest request;

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::GetAllAppKeysResponse response;
        grpc::Status status = _stub->GetAllAppKeys(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: GetAllAppKeys failed (endpoint=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outKeys.reserve(static_cast<size_t>(response.app_keys_size()));
        for (const auto& k : response.app_keys()) {
            outKeys.emplace_back(k);
        }

        LOGINFO("PermissionsClient: GetAllAppKeys ok (endpoint=%s, count=%zu)",
                _endpoint.c_str(), outKeys.size());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::GetThorToken(const std::string& app,
                                             const std::string& contentProvider,
                                             const std::string& deviceSessionId,
                                             const std::string& appSessionId,
                                             const std::string& tokenMode,
                                             int32_t ttl,
                                             const std::string& bearerToken,
                                             const std::string& deviceId,
                                             const std::string& accountId,
                                             const std::string& partnerId,
                                             std::string& outToken) const
    {
        outToken.clear();

        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::GetThorTokenRequest request;
        request.set_app(app);
        request.set_content_provider(contentProvider);
        request.set_device_session_id(deviceSessionId);
        request.set_app_session_id(appSessionId);
        request.set_token_mode(tokenMode);
        request.set_ttl(ttl);

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::GetThorTokenResponse response;
        grpc::Status status = _stub->GetThorToken(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: GetThorToken failed (endpoint=%s, app=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), app.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outToken = response.token();
        LOGINFO("PermissionsClient: GetThorToken ok (endpoint=%s, app=%s, token=[REDACTED])",
                _endpoint.c_str(), app.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::GetRegisteredPermissions(const std::string& bearerToken,
                                                         const std::string& deviceId,
                                                         const std::string& accountId,
                                                         const std::string& partnerId,
                                                         std::vector<ottx::permission::Permission>& outPermissions) const
    {
        outPermissions.clear();

        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::GetRegisteredPermissionsRequest request;

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::GetRegisteredPermissionsResponse response;
        grpc::Status status = _stub->GetRegisteredPermissions(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: GetRegisteredPermissions failed (endpoint=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        outPermissions.reserve(static_cast<size_t>(response.permissions_size()));
        for (const auto& p : response.permissions()) {
            outPermissions.emplace_back(p);
        }

        LOGINFO("PermissionsClient: GetRegisteredPermissions ok (endpoint=%s, count=%zu)",
                _endpoint.c_str(), outPermissions.size());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::RegisterPermissions(const std::vector<ottx::permission::Permission>& permissions,
                                                    bool replaceAll,
                                                    const std::string& bearerToken,
                                                    const std::string& deviceId,
                                                    const std::string& accountId,
                                                    const std::string& partnerId) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::RegisterPermissionsRequest request;
        for (const auto& p : permissions) {
            *request.add_permissions() = p;
        }
        request.set_replace_all(replaceAll);

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::RegisterPermissionsResponse response;
        grpc::Status status = _stub->RegisterPermissions(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: RegisterPermissions failed (endpoint=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        LOGINFO("PermissionsClient: RegisterPermissions ok (endpoint=%s)", _endpoint.c_str());
        return Core::ERROR_NONE;
    }

    uint32_t PermissionsClient::DeleteRegisteredPermission(const ottx::permission::Permission& permission,
                                                           const std::string& bearerToken,
                                                           const std::string& deviceId,
                                                           const std::string& accountId,
                                                           const std::string& partnerId) const
    {
        if (!_stub) {
            LOGERR("PermissionsClient: gRPC stub not initialized (endpoint=%s)", _endpoint.c_str());
            return Core::ERROR_UNAVAILABLE;
        }

        ottx::permission::DeleteRegisteredPermissionRequest request;
        *request.mutable_permission() = permission;

        grpc::ClientContext ctx;
        SetCommonMetadata(ctx, bearerToken, deviceId, accountId, partnerId);

        ottx::permission::DeleteRegisteredPermissionResponse response;
        grpc::Status status = _stub->DeleteRegisteredPermission(&ctx, request, &response);
        if (!status.ok()) {
            const uint32_t rc = MapGrpcStatusToCore(status);
            LOGERR("PermissionsClient: DeleteRegisteredPermission failed (endpoint=%s, code=%d, msg=%s)",
                   _endpoint.c_str(), static_cast<int>(status.error_code()),
                   status.error_message().c_str());
            return rc;
        }

        LOGINFO("PermissionsClient: DeleteRegisteredPermission ok (endpoint=%s)", _endpoint.c_str());
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
