#include "TokenClient.h"

#include <core/JSON.h>
#include "UtilsLogging.h"

#if defined(OTTSERVICES_ENABLE_OTT_TOKEN)
#include <grpcpp/grpcpp.h>
#include "ott_token.pb.h"
#include "ott_token.grpc.pb.h"
#endif

namespace WPEFramework {
namespace Plugin {

    namespace {
        inline std::string QuoteJson(const std::string& value) {
            Core::JSON::String s;
            s = value;
            std::string out;
            s.ToString(out); // produces a JSON string with proper quoting
            return out;
        }

    #if defined(OTTSERVICES_ENABLE_OTT_TOKEN)
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
    #endif
    } // namespace

    TokenClient::TokenClient(const std::string& endpoint, bool useTls)
        : _endpoint(endpoint), _useTls(useTls) {
    }

    TokenClient::~TokenClient() = default;

    bool TokenClient::GetPlatformToken(const std::string& appId,
                                       const std::string& xact,
                                       const std::string& sat,
                                       std::string& outTokenJson,
                                       std::string& errMsg)
    {
        LOGINFO("TokenClient::GetPlatformToken (appId='%s', endpoint=%s)", appId.c_str(), _endpoint.c_str());
        outTokenJson.clear();
        errMsg.clear();

        if (appId.empty() || sat.empty()) {
            errMsg = "Invalid arguments: 'appId' and 'sat' are required";
            return false;
        }

    #if !defined(OTTSERVICES_ENABLE_OTT_TOKEN)
        VARIABLE_IS_NOT_USED std::string _xact = xact;
        VARIABLE_IS_NOT_USED std::string _sat  = sat;
        errMsg = "ott_token support disabled at build time";
        return false;
    #else
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

        // Build JSON safely using quoted values
        outTokenJson.reserve(256);
        outTokenJson  = "{";
        outTokenJson += "\"platform_token\":" + QuoteJson(response.platform_token()) + ",";
        outTokenJson += "\"token_type\":"     + QuoteJson(response.token_type())     + ",";
        outTokenJson += "\"scope\":"          + QuoteJson(response.scope())          + ",";
        outTokenJson += "\"expires_in\":"     + std::to_string(response.expires_in());
        outTokenJson += "}";

        LOGINFO("TokenClient PlatformToken success (endpoint=%s, token=[REDACTED])", _endpoint.c_str());
        return true;
    #endif
    }

    bool TokenClient::GetAuthToken(const std::string& appId,
                                   const std::string& sat,
                                   std::string& outTokenJson,
                                   std::string& errMsg)
    {
        LOGINFO("TokenClient::GetAuthToken (appId='%s', endpoint=%s)", appId.c_str(), _endpoint.c_str());
        outTokenJson.clear();
        errMsg.clear();

        if (appId.empty() || sat.empty()) {
            errMsg = "Invalid arguments: 'appId' and 'sat' are required";
            return false;
        }

    #if !defined(OTTSERVICES_ENABLE_OTT_TOKEN)
        VARIABLE_IS_NOT_USED std::string _sat = sat;
        errMsg = "ott_token support disabled at build time";
        return false;
    #else
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

        outTokenJson.reserve(256);
        outTokenJson  = "{";
        outTokenJson += "\"access_token\":" + QuoteJson(response.access_token()) + ",";
        outTokenJson += "\"token_type\":"   + QuoteJson(response.token_type())   + ",";
        outTokenJson += "\"scope\":"        + QuoteJson(response.scope())        + ",";
        outTokenJson += "\"expires_in\":"   + std::to_string(response.expires_in());
        outTokenJson += "}";

        LOGINFO("TokenClient AuthToken success (endpoint=%s, token=[REDACTED])", _endpoint.c_str());
        return true;
    #endif
    }

} // namespace Plugin
} // namespace WPEFramework
