#pragma once

// Exchange interface for the OttServices plugin, exposed via COMRPC.
// This interface allows callers to invoke OttServices functionality through COMRPC,
// leveraging WPEFramework's Core::IUnknown and RPC infrastructure.

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // PUBLIC_INTERFACE
    struct EXTERNAL IOttServices : virtual public Core::IUnknown {
        /**
         * OttServices COMRPC interface.
         * Methods map to the plugin's supported features (ping, permissions cache operations).
         */
        enum { ID = ID_OTT_SERVICES };

        virtual ~IOttServices() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult Ping(const string& message, string& reply) = 0;
        /** Ping the service and obtain an echo reply.
         * @param message Input message. May be empty.
         * @param reply Output string with a "pong" style response.
         * @return Core::ERROR_NONE on success.
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPermissions(const string& appId, string& permissionsJson) = 0;
        /** Get permissions for the given appId as a JSON array string.
         * Example: ["perm.read","perm.write"]
         * @param appId Application identifier.
         * @param permissionsJson Output JSON array string of permissions.
         * @return Core::ERROR_NONE on success.
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult InvalidatePermissions(const string& appId) = 0;
        /** Invalidate any cached permissions for the given appId.
         * @param appId Application identifier.
         * @return Core::ERROR_NONE on success.
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult UpdatePermissionsCache(const string& appId, uint32_t& updatedCount) = 0;
        /** Fetch permissions from the remote service and update the cache for appId.
         * @param appId Application identifier.
         * @param updatedCount Output number of permissions retrieved.
         * @return Core::ERROR_NONE on success.
         */
    };

} // namespace Exchange
} // namespace WPEFramework
