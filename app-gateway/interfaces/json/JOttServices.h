#pragma once

/*
 * JSON-RPC registration helpers for IOttServices.
 * Updated: ott.getDistributorToken and ott.getAuthToken no longer accept xact/sat.
 * OttServices now retrieves SAT and xACT internally via Thunder plugins.
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
            // params: { "appId": "<firebolt appId>" }
            parent.Register(_T("ott.getDistributorToken"),
                [api](const Core::JSONRPC::Context& /*ctx*/, const string& /*method*/, const string& parameters, string& response) -> uint32_t {
                    Core::JSON::VariantContainer input;
                    input.FromString(parameters);

                    string appId;
                    if (input.HasLabel(_T("appId"))) { appId = input[_T("appId")].String(); }

                    if (appId.empty()) {
                        return Core::ERROR_BAD_REQUEST;
                    }

                    string tokenJson;
                    const Core::hresult rc = api->GetDistributorToken(appId, tokenJson);

                    if (rc == Core::ERROR_NONE) {
                        response = tokenJson.empty() ? _T("{}") : tokenJson;
                    }

                    return rc;
                });

            // ott.getAuthToken
            // params: { "appId": "<firebolt appId>" }
            parent.Register(_T("ott.getAuthToken"),
                [api](const Core::JSONRPC::Context& /*ctx*/, const string& /*method*/, const string& parameters, string& response) -> uint32_t {
                    Core::JSON::VariantContainer input;
                    input.FromString(parameters);

                    string appId;
                    if (input.HasLabel(_T("appId"))) { appId = input[_T("appId")].String(); }

                    if (appId.empty()) {
                        return Core::ERROR_BAD_REQUEST;
                    }

                    string tokenJson;
                    const Core::hresult rc = api->GetAuthToken(appId, tokenJson);

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
