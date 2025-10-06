#pragma once

// UtilsJsonrpcDirectLink.h
// Minimal stub utility to provide a JSON-RPC direct link handle for Thunder.
// This implementation intentionally returns nullptr so callers can gracefully
// handle unavailability. The JSONRPCDirectLink class offers an Invoke method
// signature to satisfy compilation if needed.

#include <core/core.h>
#include <plugins/IShell.h>
#include <memory>

namespace WPEFramework {
namespace Utils {

    // PUBLIC_INTERFACE
    class JSONRPCDirectLink {
    public:
        /** Minimal stub that exposes an Invoke method compatible with expected usage. */
        JSONRPCDirectLink() = default;
        ~JSONRPCDirectLink() = default;

        template <typename IN, typename OUT>
        uint32_t Invoke(const string& /*method*/, const IN& /*params*/, OUT& /*response*/)
        {
            return Core::ERROR_UNAVAILABLE;
        }
    };

    // Backwards compatibility alias if any code still references JsonRpcDirectLink
    using JsonRpcDirectLink = JSONRPCDirectLink;

    // PUBLIC_INTERFACE
    inline std::shared_ptr<JSONRPCDirectLink> GetThunderControllerClient(PluginHost::IShell* /*service*/, const string& /*callsign*/)
    {
        // Return nullptr to indicate that a direct link is not available in this build context.
        return nullptr;
    }

} // namespace Utils
} // namespace WPEFramework
