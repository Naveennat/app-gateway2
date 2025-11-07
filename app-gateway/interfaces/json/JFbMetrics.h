#pragma once

/*
 * Minimal stub for JFbMetrics JSON-RPC registration helpers.
 * Matches the pattern of other J* stubs in this project.
 */

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IFbMetrics;

    struct JFbMetrics {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& /*parent*/, IFbMetrics* /*api*/) {
            /** Register JSON-RPC methods for IFbMetrics on the given dispatcher (no-op stub). */
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& /*parent*/) {
            /** Unregister JSON-RPC methods for IFbMetrics from the given dispatcher (no-op stub). */
        }
    };

} // namespace Exchange
} // namespace WPEFramework
