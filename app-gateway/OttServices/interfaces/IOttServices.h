#pragma once
#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    // @json 1.0.0 @text:keep
    struct EXTERNAL IOttPermissions : virtual public Core::IUnknown {
       
        enum { ID = ID_OTT_PERMISSIONS };

        virtual ~IOttPermissions() override = default;

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

        // @text getDistributorToken
        // @brief Retrieve a distributor/platform token for the given application identifier.
        // @param appId: Application identifier for which to retrieve the token.
        // @param token: Output raw token string.
        // @returns Core::hresult
        virtual Core::hresult GetDistributorToken(const string& appId, string& token /* @out */) = 0;

        // @text getAuthToken
        // @brief Retrieve an auth token for the given application identifier.
        // @param appId: Application identifier for which to retrieve the auth token.
        // @param token: Output raw auth token string.
        // @returns Core::hresult
        virtual Core::hresult GetAuthToken(const string& appId, string& token /* @out */) = 0;
    };

} // namespace Exchange
} // namespace WPEFramework
