#pragma once

/*
 * JSON-RPC registration helpers for IOttServices.
 * Adds initial ott.getDistributorToken and ott.getAuthToken method shims.
 * Handlers call into the COMRPC interface and return placeholder data or errors.
 */

#include <plugins/JSONRPC.h>
#include <core/JSON.h>
#include <string>

namespace WPEFramework {
namespace PluginHost {
    class JSONRPC;
}
namespace Exchange {

    struct IOttServices;

    struct JOttServices {
        // PUBLIC_INTERFACE
        static void Register(PluginHost::JSONRPC& parent, IOttServices* api) {
            if (api == nullptr) {
                return;
            }

            // ott.getDistributorToken
            parent.Register(_T("ott.getDistributorToken"),
                [api](const Core::JSONRPC::Context& /*ctx*/, const string& /*method*/, const string& parameters, string& response) -> uint32_t {
                    Core::JSON::VariantContainer input;
                    input.FromString(parameters);

                    string appId;
                    string xact;
                    string sat;

                    if (input.HasLabel(_T("appId"))) { appId = input[_T("appId")].String(); }
                    if (input.HasLabel(_T("xact")))  { xact  = input[_T("xact")].String();  }
                    if (input.HasLabel(_T("sat")))   { sat   = input[_T("sat")].String();   }

                    if (appId.empty() || sat.empty()) {
                        return Core::ERROR_BAD_REQUEST;
                    }

                    string tokenJson;
                    const Core::hresult rc = api->GetDistributorToken(appId, xact, sat, tokenJson);

                    if (rc == Core::ERROR_NONE) {
                        // If tokenJson is a JSON object string, return it directly.
                        response = tokenJson.empty() ? _T("{}") : tokenJson;
                    }

                    return rc;
                });

            // ott.getAuthToken
            parent.Register(_T("ott.getAuthToken"),
                [api](const Core::JSONRPC::Context& /*ctx*/, const string& /*method*/, const string& parameters, string& response) -> uint32_t {
                    Core::JSON::VariantContainer input;
                    input.FromString(parameters);

                    string appId;
                    string sat;

                    if (input.HasLabel(_T("appId"))) { appId = input[_T("appId")].String(); }
                    if (input.HasLabel(_T("sat")))   { sat   = input[_T("sat")].String();   }

                    if (appId.empty() || sat.empty()) {
                        return Core::ERROR_BAD_REQUEST;
                    }

                    string tokenJson;
                    const Core::hresult rc = api->GetAuthToken(appId, sat, tokenJson);

                    if (rc == Core::ERROR_NONE) {
                        response = tokenJson.empty() ? _T("{}") : tokenJson;
                    }

                    return rc;
                });
        }

        // PUBLIC_INTERFACE
        static void Unregister(PluginHost::JSONRPC& parent) {
            parent.Unregister(_T("ott.getDistributorToken"));
            parent.Unregister(_T("ott.getAuthToken"));
        }
    };

} // namespace Exchange
} // namespace WPEFramework
