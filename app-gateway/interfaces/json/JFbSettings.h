#pragma once

/*
 * Lightweight local stub for JFbSettings JSON-RPC registration helpers.
 * This project does not ship the generated Thunder headers for FbSettings,
 * so we provide minimal no-op implementations to satisfy compilation.
 */

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IFbSettings;

    struct JFbSettings {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& /*parent*/, IFbSettings* /*api*/) {
            /** Register JSON-RPC methods for IFbSettings on the given dispatcher (no-op stub). */
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& /*parent*/) {
            /** Unregister JSON-RPC methods for IFbSettings from the given dispatcher (no-op stub). */
        }
    };

} // namespace Exchange
} // namespace WPEFramework
