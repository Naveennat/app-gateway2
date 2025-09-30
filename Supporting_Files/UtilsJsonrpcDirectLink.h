#pragma once

// UtilsJsonrpcDirectLink.h
// Minimal stub utility to provide a JSON-RPC direct link handle for Thunder.
// This implementation intentionally returns nullptr so callers can gracefully
// handle unavailability. The JsonRpcDirectLink class offers an Invoke method
// signature to satisfy compilation if needed.

#include <core/core.h>
#include <plugins/IShell.h>
#include <memory>

namespace WPEFramework {
namespace Utils {

    class JsonRpcDirectLink {
    public:
        JsonRpcDirectLink() = default;
        ~JsonRpcDirectLink() = default;

        template <typename IN, typename OUT>
        uint32_t Invoke(const string& /*method*/, const IN& /*params*/, OUT& /*response*/)
        {
            return Core::ERROR_UNAVAILABLE;
        }
    };

    // PUBLIC_INTERFACE
    inline std::unique_ptr<JsonRpcDirectLink> GetThunderControllerClient(PluginHost::IShell* /*service*/, const string& /*callsign*/)
    {
        // Return nullptr to indicate that a direct link is not available in this build context.
        return nullptr;
    }

} // namespace Utils
} // namespace WPEFramework
