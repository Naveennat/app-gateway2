#include <cassert>
#include <iostream>
#include <string>

// Pull in AppGateway headers and Thunder JSON types
#include "../AppGateway/AppGateway.h"
#include "../AppGateway/Resolver.h"
#include "../AppGateway/ResolutionStore.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

static const uint32_t JSONRPC_INVALID_PATH = 2;
static const uint32_t JSONRPC_PARSE_ERROR = static_cast<uint32_t>(-32700);
static const uint32_t JSONRPC_INVALID_PARAMS = static_cast<uint32_t>(-32602);
static const uint32_t JSONRPC_INVALID_REQUEST = static_cast<uint32_t>(-32699);

// Helper to simulate ConfigureParams
static AppGateway::Config MakeDefaultConfig() {
    AppGateway::Config cfg;
    cfg.ServerPort = 0; // Avoid starting any WS server in tests
    cfg.PermissionEnforcement = true;
    cfg.JwtEnabled = false;
    cfg.GatewayCallsign = "org.rdk.AppGateway";
    // No resolution paths by default here
    return cfg;
}

int main() {
    int failures = 0;

    // Instantiate gateway (without an actual shell)
    AppGateway gateway;

    // --- Test: configure with missing/empty paths -> INVALID_PARAMS as per design ---
    {
        AppGateway::ConfigureParams params;
        Core::JSON::Container response;

        uint32_t rc = gateway.endpoint_configure(params, response);
        if (rc != JSONRPC_INVALID_PARAMS) {
            std::cerr << "FAIL: configure without paths should return INVALID_PARAMS, got " << rc << std::endl;
            failures++;
        }

        // Also build an empty array explicitly
        Core::JSON::String dummy;
        params.Paths.Add(dummy); // empty string
        rc = gateway.endpoint_configure(params, response);
        if (rc != JSONRPC_INVALID_PARAMS) {
            std::cerr << "FAIL: configure with only empty path should return INVALID_PARAMS, got " << rc << std::endl;
            failures++;
        }
    }

    // --- Test: configure with invalid path -> INVALID_PATH (2) ---
    {
        AppGateway::ConfigureParams params;
        Core::JSON::String s;
        s = "/no/such/file.json";
        params.Paths.Add(s);

        Core::JSON::Container response;
        uint32_t rc = gateway.endpoint_configure(params, response);
        if (rc != JSONRPC_INVALID_PATH) {
            std::cerr << "FAIL: configure invalid path should return INVALID_PATH (2), got " << rc << std::endl;
            failures++;
        }
    }

    // --- Test: respond with invalid context -> INVALID_PARAMS ---
    {
        AppGateway::RespondParams params;
        // Missing context fields - only set a payload to see context validation fail
        params.Payload[_T("result")] = Core::JSON::Object(); // Any payload

        Core::JSON::Container response;
        uint32_t rc = gateway.endpoint_respond(params, response);
        if (rc != JSONRPC_INVALID_PARAMS) {
            std::cerr << "FAIL: respond missing context should return INVALID_PARAMS, got " << rc << std::endl;
            failures++;
        }
    }

    // --- Test: respond with legacy result/error wrapping accepted ---
    {
        AppGateway::RespondParams params;
        params.Ctx.RequestId = 1;
        params.Ctx.ConnectionId = "conn-1";
        params.Ctx.AppId = "app-1";

        // Don't set payload; only set legacy result
        Core::JSON::Object res;
        res[_T("ok")] = Core::JSON::Variant(true);
        params.Result = res;

        Core::JSON::Container response;
        uint32_t rc = gateway.endpoint_respond(params, response);
        if (rc != Core::ERROR_NONE) {
            std::cerr << "FAIL: respond with legacy result should succeed, got " << rc << std::endl;
            failures++;
        }
    }

    // --- Test: resolve invalid params (no method) -> INVALID_PARAMS ---
    {
        AppGateway::ResolveParams params;
        AppGateway::ResolveResult response;

        // no method set
        uint32_t rc = gateway.endpoint_resolve(params, response);
        if (rc != JSONRPC_INVALID_PARAMS) {
            std::cerr << "FAIL: resolve without method should return INVALID_PARAMS, got " << rc << std::endl;
            failures++;
        }
    }

    // --- Test: resolve fallback behavior when no resolution exists -> echo alias ---
    {
        AppGateway::ResolveParams params;
        params.Method = "Privacy.setAllowWatchHistory"; // no store loaded, expect fallback alias to method itself
        AppGateway::ResolveResult response;

        uint32_t rc = gateway.endpoint_resolve(params, response);
        if (rc != Core::ERROR_NONE) {
            std::cerr << "FAIL: resolve should succeed with fallback alias, rc=" << rc << std::endl;
            failures++;
        } else {
            // Verify response contains a "resolution" object with alias == method
            Core::JSON::Object resolution = response.Resolution;
            if (!resolution.HasLabel(_T("alias"))) {
                std::cerr << "FAIL: resolution should contain alias" << std::endl;
                failures++;
            } else {
                auto v = resolution.Get(_T("alias"));
                if (v.Content() != Core::JSON::Variant::type::STRING) {
                    std::cerr << "FAIL: resolution.alias should be string" << std::endl;
                    failures++;
                } else if (v.String() != params.Method.Value()) {
                    std::cerr << "FAIL: resolution.alias mismatch. expected=" << params.Method.Value()
                              << " got=" << v.String() << std::endl;
                    failures++;
                }
            }
        }
    }

    // --- Test: Resolver/ResolutionStore overlay semantics (last-wins) ---
    {
        // Prepare two overlay fragments as strings to feed into ResolutionStore via Resolver::LoadPaths
        // Since Resolver::LoadPaths reads files, we will test ResolutionStore directly here.

        ResolutionStore store;
        // First overlay with alias A
        {
            Core::JSON::Object resA;
            resA[_T("alias")] = Core::JSON::Variant("org.rdk.PluginA.method");
            std::unordered_map<string, Core::JSON::Object> addA;
            addA["device.uid"] = resA;
            store.Overlay(addA);
        }
        // Second overlay with alias B that should override A
        {
            Core::JSON::Object resB;
            resB[_T("alias")] = Core::JSON::Variant("org.rdk.PluginB.method");
            std::unordered_map<string, Core::JSON::Object> addB;
            addB["device.uid"] = resB;
            store.Overlay(addB);
        }

        Core::JSON::Object out;
        bool got = store.Get("anyApp", "device.uid", Core::JSON::Object(), out);
        if (!got) {
            std::cerr << "FAIL: ResolutionStore.Get should return true for device.uid" << std::endl;
            failures++;
        } else {
            auto v = out.Get(_T("alias"));
            if (v.Content() != Core::JSON::Variant::type::STRING || v.String() != "org.rdk.PluginB.method") {
                std::cerr << "FAIL: ResolutionStore last-wins overlay failed. alias=" << v.String() << std::endl;
                failures++;
            }
        }
    }

    if (failures == 0) {
        std::cout << "All AppGateway tests passed." << std::endl;
    } else {
        std::cerr << failures << " AppGateway test(s) failed." << std::endl;
    }

    return failures == 0 ? 0 : 1;
}
