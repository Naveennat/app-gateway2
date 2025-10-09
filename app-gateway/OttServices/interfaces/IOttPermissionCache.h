#pragma once

// Internal interface for a permission cache used by the OttServices plugin.
// Minimal interface intended for local consumption only (no JSON-RPC exposure).
// Follows the Exchange interface style by inheriting Core::IUnknown so it can
// participate in the interface map (BEGIN_INTERFACE_MAP/INTERFACE_ENTRY/END_INTERFACE_MAP).

#include "Module.h"

#include <vector>
#include <string>

namespace WPEFramework {
namespace Exchange {

    struct EXTERNAL IOttPermissionCache : virtual public Core::IUnknown {
        // Unique interface ID defined in OttServices/Module.h
        enum { ID = ID_OTT_PERMISSION_CACHE };

        virtual ~IOttPermissionCache() = default;

        // PUBLIC_INTERFACE
        virtual std::vector<string> GetPermissions(const string& appId) = 0;
        /** Retrieve permissions for the specified application identifier.
         *  @param appId Application identifier string.
         *  @return Vector of permission strings or empty vector if not present.
         */

        // PUBLIC_INTERFACE
        virtual void UpdateCache(const string& appId, const std::vector<string>& permissions) = 0;
        /** Replace cached permissions for the specified appId.
         *  @param appId Application identifier string.
         *  @param permissions Vector of permission strings.
         */

        // PUBLIC_INTERFACE
        virtual void Invalidate(const string& appId) = 0;
        /** Remove any cached permissions for appId. */

        // PUBLIC_INTERFACE
        virtual void Clear() = 0;
        /** Clear the entire cache. */

        // PUBLIC_INTERFACE
        virtual bool Has(const string& appId) const = 0;
        /** Check if cache contains an entry for appId. */

        // PUBLIC_INTERFACE
        virtual size_t Size() const = 0;
        /** Get current number of cached appId entries. */
    };

} // namespace Exchange
} // namespace WPEFramework
