#pragma once

// TokenClient: thin facade for future ott_token.proto gRPC client.
// Currently a stub that logs and returns false for all operations.

#include <string>

namespace WPEFramework {
namespace Plugin {

    class TokenClient {
    public:
        // PUBLIC_INTERFACE
        TokenClient() = default;
        ~TokenClient() = default;

        // PUBLIC_INTERFACE
        // Attempts to retrieve a platform/distributor token.
        // Returns false and sets errMsg until implemented.
        bool GetPlatformToken(const std::string& appId,
                              const std::string& xact,
                              const std::string& sat,
                              std::string& outTokenJson,
                              std::string& errMsg);

        // PUBLIC_INTERFACE
        // Attempts to retrieve an auth token.
        // Returns false and sets errMsg until implemented.
        bool GetAuthToken(const std::string& appId,
                          const std::string& sat,
                          std::string& outTokenJson,
                          std::string& errMsg);
    };

} // namespace Plugin
} // namespace WPEFramework
