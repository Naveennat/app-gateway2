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

// PermissionsClient.h
// Thin wrapper around gRPC stubs generated from OttServices/permission_service.proto
// Provides a convenient API for OttPermissionsImplementation to enumerate permissions for an app and
// to call all AppPermissionsService RPCs.
//
// NOTE: Follows the same style as EnumeratePermissions: request building, metadata, error mapping,
//       logging redaction, and Core::hresult return codes.

#include <string>
#include <vector>
#include <memory>

#include <core/core.h>

#include <grpcpp/grpcpp.h>
// Generated headers from permission_service.proto (generated via CMake custom commands)
#include "permission_service.pb.h"
#include "permission_service.grpc.pb.h"

// OttServices-wide defaults for per-RPC deadlines and channel idle timeout.
#include "../../Module.h"

namespace WPEFramework {
namespace Plugin {

    // PUBLIC_INTERFACE
    class PermissionsClient {
    public:
        /**
         * PUBLIC_INTERFACE
         * Construct a PermissionsClient that targets a permission service endpoint.
         *
         * Parameters:
         * - endpoint: "host:port" of the remote gRPC permission service. Example: "thor-permission.svc.thor.comcast.com:443".
         * - useTls: If true, use TLS channel credentials. If false, use insecure channel credentials.
         */
        /**
         * PUBLIC_INTERFACE
         * Construct a PermissionsClient that targets a permission service endpoint.
         *
         * Parameters:
         * - endpoint: "host:port" of the remote gRPC permission service. Example: "thor-permission.svc.thor.comcast.com:443".
         * - useTls: If true, use TLS channel credentials. If false, use insecure channel credentials.
         * - grpcTimeoutMs: Per-RPC deadline in milliseconds (defaults to GRPC_TIMEOUT).
         * - idleTimeoutMs: Channel idle timeout in milliseconds (defaults to IDLE_TIMEOUT).
         */
        // PUBLIC_INTERFACE
        explicit PermissionsClient(const std::string& endpoint,
                                   bool useTls = true,
                                   uint32_t grpcTimeoutMs = GRPC_TIMEOUT,
                                   uint32_t idleTimeoutMs = IDLE_TIMEOUT);

        /**
         * PUBLIC_INTERFACE
         * Advanced constructor allowing explicit channel credentials.
         *
         * Parameters:
         * - endpoint: "host:port" of the remote gRPC permission service.
         * - creds: Prebuilt grpc::ChannelCredentials (e.g., grpc::SslCredentials or grpc::InsecureChannelCredentials()).
         * - grpcTimeoutMs: Per-RPC deadline in milliseconds (defaults to GRPC_TIMEOUT).
         * - idleTimeoutMs: Channel idle timeout in milliseconds (defaults to IDLE_TIMEOUT).
         */
        // PUBLIC_INTERFACE
        PermissionsClient(const std::string& endpoint,
                          std::shared_ptr<grpc::ChannelCredentials> creds,
                          uint32_t grpcTimeoutMs = GRPC_TIMEOUT,
                          uint32_t idleTimeoutMs = IDLE_TIMEOUT);

        // PUBLIC_INTERFACE
        ~PermissionsClient();

        /**
         * PUBLIC_INTERFACE
         * Enumerate permissions for an app using the remote AppPermissionsService.
         */
        // PUBLIC_INTERFACE
        uint32_t EnumeratePermissions(const std::string& appId,
                                      const std::string& partnerId,
                                      const std::string& bearerToken,
                                      const std::string& deviceId,
                                      const std::string& accountId,
                                      const std::vector<std::string>& filters,
                                      std::vector<std::string>& outPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Get permissions object for an app (Get RPC).
         *
         * Output:
         * - outAppPermissions: Populated on success.
         */
        // PUBLIC_INTERFACE
        uint32_t Get(const std::string& appId,
                     const std::string& partnerId,
                     const std::string& bearerToken,
                     const std::string& deviceId,
                     const std::string& accountId,
                     ottx::permission::ApplicationPermissions& outAppPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Grant permissions/bundles to an app (Grant RPC).
         *
         * Inputs:
         * - bundles, permissions: lists to be granted (either may be empty).
         * Output: outAppPermissions reflects resulting permissions.
         */
        // PUBLIC_INTERFACE
        uint32_t Grant(const std::string& appId,
                       const std::string& partnerId,
                       const std::vector<std::string>& bundles,
                       const std::vector<std::string>& permissions,
                       const std::string& bearerToken,
                       const std::string& deviceId,
                       const std::string& accountId,
                       ottx::permission::ApplicationPermissions& outAppPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Set the permissions object for an app (Set RPC).
         *
         * Inputs:
         * - bundles, includes, excludes: full replacement of the app permissions vectors.
         */
        // PUBLIC_INTERFACE
        uint32_t Set(const std::string& appId,
                     const std::string& partnerId,
                     const std::vector<std::string>& bundles,
                     const std::vector<std::string>& includes,
                     const std::vector<std::string>& excludes,
                     const std::string& bearerToken,
                     const std::string& deviceId,
                     const std::string& accountId,
                     ottx::permission::ApplicationPermissions& outAppPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Revoke permissions/bundles from an app (Revoke RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t Revoke(const std::string& appId,
                        const std::string& partnerId,
                        const std::vector<std::string>& bundles,
                        const std::vector<std::string>& permissions,
                        const std::string& bearerToken,
                        const std::string& deviceId,
                        const std::string& accountId,
                        ottx::permission::ApplicationPermissions& outAppPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Delete an app's permission record (Delete RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t Delete(const std::string& appId,
                        const std::string& partnerId,
                        const std::string& bearerToken,
                        const std::string& deviceId,
                        const std::string& accountId) const;

        /**
         * PUBLIC_INTERFACE
         * Authorize a session token against filters (Authorize RPC).
         *
         * Input:
         * - sessionToken: Token to evaluate.
         * - filters: permission filter strings to check.
         * Output:
         * - outPermissions: resulting permission strings granted by authorization.
         */
        // PUBLIC_INTERFACE
        uint32_t Authorize(const std::string& sessionToken,
                           const std::vector<std::string>& filters,
                           const std::string& bearerToken,
                           const std::string& deviceId,
                           const std::string& accountId,
                           const std::string& partnerId,
                           std::vector<std::string>& outPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Get all permission entries for an app (GetAllForApp RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t GetAllForApp(const std::string& app,
                              const std::string& bearerToken,
                              const std::string& deviceId,
                              const std::string& accountId,
                              const std::string& partnerId,
                              std::vector<ottx::permission::ApplicationPermissions>& outList) const;

        /**
         * PUBLIC_INTERFACE
         * Get all app keys known to the service (GetAllAppKeys RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t GetAllAppKeys(const std::string& bearerToken,
                               const std::string& deviceId,
                               const std::string& accountId,
                               const std::string& partnerId,
                               std::vector<ottx::permission::AppKeys>& outKeys) const;

        /**
         * PUBLIC_INTERFACE
         * Acquire a Thor token (GetThorToken RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t GetThorTokenRPC(const std::string& app,
                              const std::string& contentProvider,
                              const std::string& deviceSessionId,
                              const std::string& appSessionId,
                              const std::string& tokenMode,
                              int32_t ttl,
                              const std::string& bearerToken,
                              const std::string& deviceId,
                              const std::string& accountId,
                              const std::string& partnerId,
                              std::string& outToken) const;

        /**
         * PUBLIC_INTERFACE
         * Get registered permission catalog (GetRegisteredPermissions RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t GetRegisteredPermissions(const std::string& bearerToken,
                                          const std::string& deviceId,
                                          const std::string& accountId,
                                          const std::string& partnerId,
                                          std::vector<ottx::permission::Permission>& outPermissions) const;

        /**
         * PUBLIC_INTERFACE
         * Register permissions in catalog (RegisterPermissions RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t RegisterPermissions(const std::vector<ottx::permission::Permission>& permissions,
                                     bool replaceAll,
                                     const std::string& bearerToken,
                                     const std::string& deviceId,
                                     const std::string& accountId,
                                     const std::string& partnerId) const;

        /**
         * PUBLIC_INTERFACE
         * Delete a registered permission from catalog (DeleteRegisteredPermission RPC).
         */
        // PUBLIC_INTERFACE
        uint32_t DeleteRegisteredPermission(const ottx::permission::Permission& permission,
                                            const std::string& bearerToken,
                                            const std::string& deviceId,
                                            const std::string& accountId,
                                            const std::string& partnerId) const;

        /**
         * PUBLIC_INTERFACE
         * Get the configured endpoint ("host:port") string for this client.
         */
        // PUBLIC_INTERFACE
        std::string Endpoint() const;

    private:
        // Apply a per-RPC deadline to the given client context using the configured timeout.
        static void ApplyDeadline(grpc::ClientContext& ctx, uint32_t grpcTimeoutMs);

        std::string _endpoint;

        // Timeout configuration (milliseconds).
        uint32_t _grpcTimeoutMs;
        uint32_t _idleTimeoutMs;

        std::shared_ptr<grpc::Channel> _channel;
        std::unique_ptr<ottx::permission::AppPermissionsService::Stub> _stub;

        static std::shared_ptr<grpc::Channel> CreateChannel(const std::string& endpoint, bool useTls, uint32_t idleTimeoutMs);
        static std::shared_ptr<grpc::Channel> CreateChannel(const std::string& endpoint,
                                                            const std::shared_ptr<grpc::ChannelCredentials>& creds,
                                                            uint32_t idleTimeoutMs);
    };

} // namespace Plugin
} // namespace WPEFramework
