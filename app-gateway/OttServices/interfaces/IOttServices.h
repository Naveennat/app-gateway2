#pragma once

// Exchange interface for the OttServices plugin, exposed via COMRPC.
// This interface allows callers to invoke OttServices functionality through COMRPC,
// leveraging WPEFramework's Core::IUnknown and RPC infrastructure.

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // @json 1.0.0 @text:keep
    struct EXTERNAL IOttServices : virtual public Core::IUnknown {
       
        enum { ID = ID_OTT_SERVICES };

        virtual ~IOttServices() override = default;

        // @text ping
        // @brief Ping the service and obtain a reply string.
        // @param message: Input message to be echoed. May be empty.
        // @param reply: Output string containing a "pong" style response.
        // @returns Core::hresult
        virtual Core::hresult Ping(const string& message, string& reply /* @out */) = 0;

        // @text getPermissions
        // @brief Retrieve permissions for the given application identifier as a JSON array string.
        // @param appId: Application identifier of the caller.
        // @param permissionsJson: Output JSON array string (e.g. ["perm.read","perm.write"]).
        // @returns Core::hresult
        virtual Core::hresult GetPermissions(const string& appId, string& permissionsJson /* @out */) = 0;

        // @text invalidatePermissions
        // @brief Invalidate any cached permissions for the given application identifier.
        // @param appId: Application identifier to invalidate in cache.
        // @returns Core::hresult
        virtual Core::hresult InvalidatePermissions(const string& appId) = 0;

        // @text updatePermissionsCache
        // @brief Fetch permissions from the remote service and update the cache for the given application.
        // @param appId: Application identifier whose permissions should be refreshed.
        // @param updatedCount: Output number of permissions retrieved/updated.
        // @returns Core::hresult
        virtual Core::hresult UpdatePermissionsCache(const string& appId, uint32_t& updatedCount /* @out */) = 0;

        // ---- Token retrieval placeholders (COMRPC) ----

        // PUBLIC_INTERFACE
        // @text getDistributorToken
        // @brief Retrieve a distributor token for the given app. Internally fetches xACT and SAT via Thunder.
        // @param appId: Application identifier (Firebolt appId).
        // @param tokenJson: Output JSON string containing the token or token envelope (opaque).
        // @returns Core::hresult
        virtual Core::hresult GetDistributorToken(const string& appId /* @in */,
                                                  string& tokenJson /* @out @opaque */) = 0;

        // PUBLIC_INTERFACE
        // @text getAuthToken
        // @brief Retrieve an auth token for the given app. Internally fetches SAT via Thunder.
        // @param appId: Application identifier (Firebolt appId).
        // @param tokenJson: Output JSON string containing the token or token envelope (opaque).
        // @returns Core::hresult
        virtual Core::hresult GetAuthToken(const string& appId /* @in */,
                                           string& tokenJson /* @out @opaque */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
