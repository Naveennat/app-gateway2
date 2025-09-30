#pragma once

// PermissionsClient.h
// Thin wrapper around gRPC stubs generated from Supporting_Files/permission_service.proto
// Provides a convenient API for OttServicesImplementation to enumerate permissions for an app.
//
// The build must define OTTSERVICES_ENABLE_PERMS_GRPC to enable the real gRPC client.
// When not defined, the client compiles into a no-op stub returning Core::ERROR_UNAVAILABLE.

#include <string>
#include <vector>

#include <core/core.h>

#if defined(OTTSERVICES_ENABLE_PERMS_GRPC)
#include <grpcpp/grpcpp.h>
// Generated headers from permission_service.proto (generated into ${CMAKE_CURRENT_BINARY_DIR})
#include "permission_service.pb.h"
#include "permission_service.grpc.pb.h"
#endif

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
         *
         * Note:
         * - It is recommended to pass the endpoint from configuration rather than hardcoding.
         * - TLS credentials use default system roots when useTls=true. Provide custom roots via grpc::SslCredentialsOptions
         *   by using the overloaded constructor below only when OTTSERVICES_ENABLE_PERMS_GRPC is defined.
         */
        // PUBLIC_INTERFACE
        explicit PermissionsClient(const std::string& endpoint, bool useTls = true);

#if defined(OTTSERVICES_ENABLE_PERMS_GRPC)
        /**
         * PUBLIC_INTERFACE
         * Advanced constructor allowing explicit channel credentials.
         *
         * Parameters:
         * - endpoint: "host:port" of the remote gRPC permission service.
         * - creds: Prebuilt grpc::ChannelCredentials (e.g., grpc::SslCredentials or grpc::InsecureChannelCredentials()).
         */
        // PUBLIC_INTERFACE
        PermissionsClient(const std::string& endpoint, std::shared_ptr<grpc::ChannelCredentials> creds);
#endif

        // PUBLIC_INTERFACE
        ~PermissionsClient();

        /**
         * PUBLIC_INTERFACE
         * Enumerate permissions for an app using the remote AppPermissionsService.
         *
         * This wraps the EnumeratePermissions RPC with proper metadata population.
         *
         * Parameters:
         * - appId: Application identifier (AppKey.app).
         * - partnerId: Syndication partner identifier (AppKey.syndication_partner_id).
         * - bearerToken: Access token (WITHOUT "Bearer " prefix); the method will prepend "Bearer ".
         * - deviceId: Device identifier to be placed into "deviceid" metadata.
         * - accountId: Service account identifier to be placed into "accountid" metadata.
         * - filters: Optional permission filter strings; may be empty.
         * - outPermissions: Output vector populated on success.
         *
         * Returns:
         * - Core::ERROR_NONE on success and outPermissions populated.
         * - Core::ERROR_UNAVAILABLE if client is not enabled/built with gRPC support.
         * - Core::ERROR_GENERAL or other Core error codes on failures.
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
         * Get the configured endpoint ("host:port") string for this client.
         */
        // PUBLIC_INTERFACE
        std::string Endpoint() const;

    private:
        std::string _endpoint;

#if defined(OTTSERVICES_ENABLE_PERMS_GRPC)
        std::shared_ptr<grpc::Channel> _channel;
        std::unique_ptr<ottx::permission::AppPermissionsService::Stub> _stub;

        static std::shared_ptr<grpc::Channel> CreateChannel(const std::string& endpoint, bool useTls);
        static std::shared_ptr<grpc::Channel> CreateChannel(const std::string& endpoint,
                                                            const std::shared_ptr<grpc::ChannelCredentials>& creds);
#endif
    };

} // namespace Plugin
} // namespace WPEFramework
