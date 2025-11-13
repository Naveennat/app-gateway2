#include "TokenClient.h"

#include "UtilsLogging.h"

#include <grpcpp/grpcpp.h>
#include "ott_token.pb.h"
#include "ott_token.grpc.pb.h"

namespace WPEFramework {
namespace Plugin {

    namespace {
        static std::shared_ptr<grpc::Channel> CreateChannel(const std::string& endpoint, bool useTls) {
            if (useTls) {
                grpc::SslCredentialsOptions ssl_opts;
                auto creds = grpc::SslCredentials(ssl_opts);
                return grpc::CreateChannel(endpoint, creds);
            } else {
                return grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials());
            }
        }

        static inline void SetSatAuth(grpc::ClientContext& ctx, const std::string& sat) {
            if (!sat.empty()) {
                ctx.AddMetadata("authorization", std::string("Bearer ") + sat);
            }
        }
    } // namespace

    TokenClient::TokenClient(const std::string& endpoint, bool useTls)
        : _endpoint(endpoint), _useTls(useTls) {
    }

    TokenClient::~TokenClient() = default;

    bool TokenClient::GetPlatformToken(const std::string& appId,
                                       const std::string& xact,
                                       const std::string& sat,
                                       std::string& outToken,
                                       uint32_t& outExpiresIn,
                                       std::string& errMsg)
    {
        LOGINFO("TokenClient::GetPlatformToken (appId='%s', endpoint=%s)", appId.c_str(), _endpoint.c_str());
        outToken.clear();
        outExpiresIn = 0;
        errMsg.clear();

        if (appId.empty() || sat.empty()) {
            errMsg = "Invalid arguments: 'appId' and 'sat' are required";
            return false;
        }

        auto channel = CreateChannel(_endpoint, _useTls);
        auto stub = ottx::otttoken::OttTokenService::NewStub(channel);

        ottx::otttoken::PlatformTokenRequest request;
        request.set_xact(xact);
        request.set_sat(sat);
        request.set_app_id(appId);

        grpc::ClientContext ctx;
        SetSatAuth(ctx, sat);

        ottx::otttoken::PlatformTokenResponse response;
        grpc::Status status = stub->PlatformToken(&ctx, request, &response);
        if (!status.ok()) {
            errMsg = std::string("gRPC error: ") + status.error_message();
            LOGERR("TokenClient PlatformToken failed (code=%d, msg=%s)", static_cast<int>(status.error_code()), status.error_message().c_str());
            return false;
        }

        outToken = response.platform_token();
        outExpiresIn = static_cast<uint32_t>(response.expires_in());
        LOGINFO("TokenClient PlatformToken success (endpoint=%s, token=[REDACTED])", _endpoint.c_str());
        return true;
    }

    bool TokenClient::GetAuthToken(const std::string& appId,
                                   const std::string& sat,
                                   std::string& outToken,
                                   uint32_t& outExpiresIn,
                                   std::string& errMsg)
    {
        LOGINFO("TokenClient::GetAuthToken (appId='%s', endpoint=%s)", appId.c_str(), _endpoint.c_str());
        outToken.clear();
        outExpiresIn = 0;
        errMsg.clear();

        if (appId.empty() || sat.empty()) {
            errMsg = "Invalid arguments: 'appId' and 'sat' are required";
            return false;
        }

        auto channel = CreateChannel(_endpoint, _useTls);
        auto stub = ottx::otttoken::OttTokenService::NewStub(channel);

        ottx::otttoken::AuthTokenRequest request;
        // partner_id and xsct are optional for this client; callers may extend when needed.
        request.set_app_id(appId);
        request.set_sat(sat);

        grpc::ClientContext ctx;
        SetSatAuth(ctx, sat);

        ottx::otttoken::AuthTokenResponse response;
        grpc::Status status = stub->AuthToken(&ctx, request, &response);
        if (!status.ok()) {
            errMsg = std::string("gRPC error: ") + status.error_message();
            LOGERR("TokenClient AuthToken failed (code=%d, msg=%s)", static_cast<int>(status.error_code()), status.error_message().c_str());
            return false;
        }

        outToken = response.access_token();
        outExpiresIn = static_cast<uint32_t>(response.expires_in());
        LOGINFO("TokenClient AuthToken success (endpoint=%s, token=[REDACTED])", _endpoint.c_str());
        return true;
    }

} // namespace Plugin
} // namespace WPEFramework
