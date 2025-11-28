#pragma once

/*
 * Lightweight local stub for JAppGatewayResolver JSON-RPC registration helpers.
 * Mirrors the pattern of other JSON stubs in this project.
 */

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IAppGatewayResolver;

    struct JAppGatewayResolver {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& /*parent*/, IAppGatewayResolver* /*api*/) {
            /** Register JSON-RPC methods for IAppGatewayResolver (no-op stub). */
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& /*parent*/) {
            /** Unregister JSON-RPC methods for IAppGatewayResolver (no-op stub). */
        }
    };

} // namespace Exchange
} // namespace WPEFramework
