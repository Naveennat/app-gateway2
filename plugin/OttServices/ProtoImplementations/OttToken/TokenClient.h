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
#pragma once

// TokenClient: thin facade for ott_token.proto gRPC client.
// If OTTSERVICES_ENABLE_OTT_TOKEN is not defined, this compiles to a stub and returns false.
//
// The real implementation creates a gRPC channel on demand and invokes:
//  - OttTokenService::PlatformToken
//  - OttTokenService::AuthToken
//
// Returned values:
//  - PlatformTokenResponse -> platform_token (string), expires_in (seconds)
//  - AuthTokenResponse     -> access_token   (string), expires_in (seconds)
//
// This client returns only the raw token string to callers (no JSON).

#include <cstdint>
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
        // Output:
        //   outToken     -> raw token string (platform_token)
        //   outExpiresIn -> expires_in value in seconds (0 if unknown)
        // Return: true on success, false on failure with errMsg filled.
        bool GetPlatformToken(const std::string& appId,
                              const std::string& xact,
                              const std::string& sat,
                              std::string& outToken,
                              uint32_t& outExpiresIn,
                              std::string& errMsg);

        // PUBLIC_INTERFACE
        // Retrieve an auth token for a partner/app.
        // Inputs: appId, sat
        // Output:
        //   outToken     -> raw token string (access_token)
        //   outExpiresIn -> expires_in value in seconds (0 if unknown)
        // Return: true on success, false on failure with errMsg filled.
        bool GetAuthToken(const std::string& appId,
                          const std::string& sat,
                          const std::string& partnerId,
                          const std::string& sct,
                          std::string& outToken,
                          uint32_t& outExpiresIn,
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
