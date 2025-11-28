#pragma once

/*
 * Lightweight local stub for JOttServices JSON-RPC registration helpers.
 * Mirrors the pattern of JFbSettings.
 */

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IOttServices;

    struct JOttServices {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& /*parent*/, IOttServices* /*api*/) {
            /** Register JSON-RPC methods for IOttServices on the given dispatcher (no-op stub). */
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& /*parent*/) {
            /** Unregister JSON-RPC methods for IOttServices from the given dispatcher (no-op stub). */
        }
    };

} // namespace Exchange
} // namespace WPEFramework
