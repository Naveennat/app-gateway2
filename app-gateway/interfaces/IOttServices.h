#pragma once

/*
 * Interface header for the Exchange::IOttServices interface.
 * Structured to align with the conventions used in FbSettings/interfaces/IFbSettings.h.
 *
 * NOTE: Method declaration conventions for this interface:
 * - Return type: Core::hresult
 * - Input scalars/strings are passed as const (by value or const ref) as currently used by implementation
 * - Output parameters are passed by non-const reference and annotated with /* @out */
 * - Public methods are marked with the PUBLIC_INTERFACE comment before declaration.
 */

#include "Module.h"

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IOttServices : virtual public Core::IUnknown {
        // Use the unique interface ID used by the OttServices plugin (defined in OttServices/Module.h).
        // Note: OttServices plugin code includes "Module.h" first, ensuring ID_OTT_SERVICES is visible.
        enum { ID = ID_OTT_SERVICES };

        virtual ~IOttServices() = default;

        // PUBLIC_INTERFACE
        virtual Core::hresult Ping(const string& message, string& reply /* @out */) = 0;
        /** Ping the service and obtain a reply string.
         *  @param message Input message to be echoed. May be empty.
         *  @param reply Output string containing a "pong" style response.
         *  @returns Core::hresult
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult GetPermissions(const string& appId, string& permissionsJson /* @out */) = 0;
        /** Retrieve permissions for the given application identifier as a JSON array string.
         *  @param appId Application identifier of the caller.
         *  @param permissionsJson Output JSON array string (e.g. ["perm.read","perm.write"]).
         *  @returns Core::hresult
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult InvalidatePermissions(const string& appId) = 0;
        /** Invalidate any cached permissions for the given application identifier.
         *  @param appId Application identifier to invalidate in cache.
         *  @returns Core::hresult
         */

        // PUBLIC_INTERFACE
        virtual Core::hresult UpdatePermissionsCache(const string& appId, uint32_t& updatedCount /* @out */) = 0;
        /** Fetch permissions from the remote service and update the cache for the given application.
         *  @param appId Application identifier whose permissions should be refreshed.
         *  @param updatedCount Output number of permissions retrieved/updated.
         *  @returns Core::hresult
         */
    };

} // namespace Exchange
} // namespace WPEFramework
