#pragma once

#include <plugins/JSONRPC.h>
#include "../IFbSettings.h"

namespace WPEFramework {
namespace Exchange {

    // Local stub for the generated JSON adapter. The real generator is not present in this
    // environment; using no-op implementations allows the plugin to compile and run.
    struct JFbSettings {
        // Register JSON-RPC endpoints against the given JSONRPC dispatcher for IFbSettings.
        static void Register(PluginHost::JSONRPC& /*server*/, IFbSettings* /*impl*/) {
            // No-op stub: in a full environment this would expose IFbSettings methods as JSONRPC.
        }

        // Unregister JSON-RPC endpoints previously registered.
        static void Unregister(PluginHost::JSONRPC& /*server*/) {
            // No-op stub.
        }
    };

} // namespace Exchange
} // namespace WPEFramework
