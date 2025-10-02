#include "PermissionsClient.h"

#include <algorithm>
#include <utility>
#include <grpcpp/security/credentials.h>

// Lightweight logging; avoid leaking secrets
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

    PermissionsClient::PermissionsClient(const std::string& endpoint, bool useTls)
        : _endpoint(endpoint)
        , _channel(CreateChannel(endpoint, useTls))
        , _stub(ottx::permission::AppPermissionsService::NewStub(_channel))
    {
        (void)useTls;
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

        ottx::permission::EnumeratePermissionsResponse response;
        grpc::Status status = _stub->EnumeratePermissions(&ctx, request, &response);

        if (!status.ok()) {
            // Map common gRPC status codes to WPEFramework errors where it makes sense
            uint32_t rc = Core::ERROR_GENERAL;
            switch (status.error_code()) {
                case grpc::StatusCode::UNAUTHENTICATED:
                case grpc::StatusCode::PERMISSION_DENIED:
                    rc = Core::ERROR_PRIVILIGED_REQUEST;
                    break;
                case grpc::StatusCode::UNAVAILABLE:
                    rc = Core::ERROR_UNAVAILABLE;
                    break;
                case grpc::StatusCode::DEADLINE_EXCEEDED:
                    rc = Core::ERROR_TIMEDOUT;
                    break;
                case grpc::StatusCode::INVALID_ARGUMENT:
                case grpc::StatusCode::FAILED_PRECONDITION:
                    rc = Core::ERROR_BAD_REQUEST;
                    break;
                default:
                    rc = Core::ERROR_GENERAL;
                    break;
            }

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

} // namespace Plugin
} // namespace WPEFramework
