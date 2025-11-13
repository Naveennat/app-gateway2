#pragma once

// TokenClient: thin facade for ott_token.proto gRPC client.
// If OTTSERVICES_ENABLE_OTT_TOKEN is not defined, this compiles to a stub and returns false.
//
// The real implementation creates a gRPC channel on demand and invokes:
//  - OttTokenService::PlatformToken
//  - OttTokenService::AuthToken
//
// Returned JSON structure mirrors the proto outputs:
//  - PlatformTokenResponse -> { "platform_token", "token_type", "scope", "expires_in" }
//  - AuthTokenResponse     -> { "access_token",   "token_type", "scope", "expires_in" }

#include <string>

namespace WPEFramework {
namespace Plugin {

    class TokenClient {
    public:
        // PUBLIC_INTERFACE
        TokenClient(const std::string& endpoint, bool useTls = true);
        /** Construct a TokenClient.
         *  endpoint: "host:port" gRPC endpoint
         *  useTls: whether to use TLS credentials for the channel
         */

        // PUBLIC_INTERFACE
        ~TokenClient();

        // PUBLIC_INTERFACE
        // Retrieve a platform/distributor token (CIMA).
        // Inputs: appId, xact, sat
        // Output: outTokenJson -> JSON string containing: platform_token, token_type, scope, expires_in
        // Return: true on success, false on failure with errMsg filled.
        bool GetPlatformToken(const std::string& appId,
                              const std::string& xact,
                              const std::string& sat,
                              std::string& outTokenJson,
                              std::string& errMsg);

        // PUBLIC_INTERFACE
        // Retrieve an auth token for a partner/app.
        // Inputs: appId, sat (AuthTokenRequest also supports partner_id/xsct but they are optional in this client)
        // Output: outTokenJson -> JSON string containing: access_token, token_type, scope, expires_in
        // Return: true on success, false on failure with errMsg filled.
        bool GetAuthToken(const std::string& appId,
                          const std::string& sat,
                          std::string& outTokenJson,
                          std::string& errMsg);

        // PUBLIC_INTERFACE
        // Return configured endpoint ("host:port")
        std::string Endpoint() const { return _endpoint; }

    private:
        std::string _endpoint;
        bool _useTls;
    };

} // namespace Plugin
} // namespace WPEFramework
