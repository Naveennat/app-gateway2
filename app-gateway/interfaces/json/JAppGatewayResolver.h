#pragma once

/*
 * Minimal JSON-RPC registration helpers for IAppGatewayResolver used by l0tests.
 * This registers a single "resolve" method that forwards to the underlying
 * Exchange::IAppGatewayResolver::Resolve implementation.
 *
 * Parameters format expected by "resolve":
 * {
 *   "requestId": <uint32>,
 *   "connectionId": <uint32>,
 *   "appId": "<string>",
 *   "origin": "<string>",            // e.g. "org.rdk.AppGateway"
 *   "method": "<string>",            // e.g. "dummy.method"
 *   "params": "<string json blob>"   // optional, defaults to "{}"
 * }
 *
 * The result is the raw JSON resolution string returned by the resolver.
 */

#include <plugins/JSONRPC.h>
#include <core/JSON.h>
#include <string>

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IAppGatewayResolver;
    struct GatewayContext;

    struct JAppGatewayResolver {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& parent, IAppGatewayResolver* api) {
            /** Register JSON-RPC method "resolve" that forwards to IAppGatewayResolver::Resolve. */
            if (api == nullptr) {
                return;
            }

            parent.Register(_T("resolve"),
                [api](const Core::JSONRPC::Context& /*ctx*/, const string& /*name*/, const string& parameters, string& response) -> uint32_t {
                    Core::JSON::VariantContainer input;
                    input.FromString(parameters);

                    Exchange::GatewayContext ctx{};
                    string origin;
                    string methodName;
                    string params("{}");

                    bool ok = true;

                    if (input.HasLabel(_T("requestId"))) {
                        // Numeric values are handled as unsigned in VariantContainer
                        ctx.requestId = static_cast<uint32_t>(input[_T("requestId")].Number());
                    } else {
                        ok = false;
                    }
                    if (input.HasLabel(_T("connectionId"))) {
                        ctx.connectionId = static_cast<uint32_t>(input[_T("connectionId")].Number());
                    } else {
                        ok = false;
                    }
                    if (input.HasLabel(_T("appId"))) {
                        ctx.appId = input[_T("appId")].String();
                    } else {
                        ok = false;
                    }
                    if (input.HasLabel(_T("origin"))) {
                        origin = input[_T("origin")].String();
                    } else {
                        ok = false;
                    }
                    if (input.HasLabel(_T("method"))) {
                        methodName = input[_T("method")].String();
                    } else {
                        ok = false;
                    }
                    if (input.HasLabel(_T("params"))) {
                        params = input[_T("params")].String();
                        if (params.empty()) {
                            params = "{}";
                        }
                    }

                    if (!ok || ctx.appId.empty() || methodName.empty()) {
                        return Core::ERROR_BAD_REQUEST;
                    }

                    string result;
                    const Core::hresult rc = api->Resolve(ctx, origin, methodName, params, result);
                    if (rc == Core::ERROR_NONE) {
                        // Pass the resolver's JSON directly as the JSON-RPC response body.
                        response = result;
                    }
                    return rc;
                });
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& parent) {
            /** Unregister "resolve" JSON-RPC method. */
            parent.Unregister(_T("resolve"));
        }
    };

} // namespace Exchange
} // namespace WPEFramework
