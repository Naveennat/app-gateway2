#pragma once

/*
 * Lightweight local stub for JAppGateway JSON-RPC registration helpers.
 * Mirrors the pattern of JFbSettings and JOttServices.
 */

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IAppGateway;

    struct JAppGateway {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& /*parent*/, IAppGateway* /*api*/) {
            /** Register JSON-RPC methods for IAppGateway on the given dispatcher (no-op stub). */
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& /*parent*/) {
            /** Unregister JSON-RPC methods for IAppGateway from the given dispatcher (no-op stub). */
        }
    };

} // namespace Exchange
} // namespace WPEFramework
