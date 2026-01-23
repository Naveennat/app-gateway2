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
#include "TokenClient.h"

#include <core/Enumerate.h>
#include <plugins/plugins.h>
#include "UtilsLogging.h"

#include <grpcpp/grpcpp.h>
#include "ott_token.pb.h"
#include "ott_token.grpc.pb.h"

namespace WPEFramework {
namespace Plugin {

    void TokenClient::ApplyDeadline(grpc::ClientContext& ctx, const uint32_t grpcTimeoutMs)
    {
        if (grpcTimeoutMs == 0) {
            return;
        }
        ctx.set_deadline(std::chrono::system_clock::now() + std::chrono::milliseconds(grpcTimeoutMs));
    }

    std::shared_ptr<grpc::Channel> TokenClient::CreateChannel(const std::string& endpoint,
                                                              const bool useTls,
                                                              const uint32_t idleTimeoutMs)
    {
        grpc::ChannelArguments args;
        if (idleTimeoutMs != 0) {
            args.SetInt(GRPC_ARG_CLIENT_IDLE_TIMEOUT_MS, static_cast<int>(idleTimeoutMs));
        }

        if (useTls) {
            grpc::SslCredentialsOptions ssl_opts;
            auto creds = grpc::SslCredentials(ssl_opts);
            return grpc::CreateCustomChannel(endpoint, creds, args);
        } else {
            return grpc::CreateCustomChannel(endpoint, grpc::InsecureChannelCredentials(), args);
        }
    }

    namespace {
        static inline void SetSatAuth(grpc::ClientContext& ctx, const std::string& sat) {
            if (!sat.empty()) {
                ctx.AddMetadata("authorization", std::string("Bearer ") + sat);
            }
        }
    } // namespace

    TokenClient::TokenClient(const std::string& endpoint,
                             const bool useTls,
                             const uint32_t grpcTimeoutMs,
                             const uint32_t idleTimeoutMs)
        : _endpoint(endpoint)
        , _useTls(useTls)
        , _grpcTimeoutMs(grpcTimeoutMs)
        , _idleTimeoutMs(idleTimeoutMs) {
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

        auto channel = TokenClient::CreateChannel(_endpoint, _useTls, _idleTimeoutMs);
        auto stub = ottx::otttoken::OttTokenService::NewStub(channel);

        ottx::otttoken::PlatformTokenRequest request;
        request.set_xact(xact);
        request.set_sat(sat);
        request.set_app_id(appId);

        grpc::ClientContext ctx;
        SetSatAuth(ctx, sat);
        ApplyDeadline(ctx, _grpcTimeoutMs);

        ottx::otttoken::PlatformTokenResponse response;
        grpc::Status status = stub->PlatformToken(&ctx, request, &response);
        if (!status.ok()) {
            if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
                LOGERR("TokenClient: DEADLINE_EXCEEDED (endpoint=%s, method=PlatformToken, deadlineMs=%u, appId=%s)",
                       _endpoint.c_str(), _grpcTimeoutMs, appId.c_str());
            }
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
                                   const std::string& partnerId,
                                   const std::string& sct,
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

        auto channel = CreateChannel(_endpoint, _useTls, _idleTimeoutMs);
        auto stub = ottx::otttoken::OttTokenService::NewStub(channel);

        ottx::otttoken::AuthTokenRequest request;
        // partner_id and xsct are optional for this client; callers may extend when needed.
        request.set_app_id(appId);
        request.set_sat(sat);
        request.set_partner_id(partnerId);
        request.set_xsct(sct);

        grpc::ClientContext ctx;
        SetSatAuth(ctx, sat);
        ApplyDeadline(ctx, _grpcTimeoutMs);

        ottx::otttoken::AuthTokenResponse response;
        grpc::Status status = stub->AuthToken(&ctx, request, &response);
        if (!status.ok()) {
            if (status.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
                LOGERR("TokenClient: DEADLINE_EXCEEDED (endpoint=%s, method=AuthToken, deadlineMs=%u, appId=%s)",
                       _endpoint.c_str(), _grpcTimeoutMs, appId.c_str());
            }
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
