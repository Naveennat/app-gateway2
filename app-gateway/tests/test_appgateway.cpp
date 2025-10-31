#include <cassert>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstdio>

#include "AppGateway/Resolver.h"
#include "AppGateway/ResolutionStore.h"
#include "AppGateway/RequestRouter.h"
#include "../helpers/JsonCompat.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

static int test_resolution_store_overlay() {
    int failures = 0;

    ResolutionStore store;
    // First overlay with alias A
    {
        Core::JSON::Object resA;
        resA[_T("alias")] = Core::JSON::Variant("org.rdk.PluginA.method");
        std::unordered_map<string, Core::JSON::Object> addA;
        addA["device.uid"] = resA;
        store.Overlay(addA);
    }
    // Second overlay with alias B that should override A (last-wins)
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

    return failures;
}

static int test_resolver_invalid_path() {
    int failures = 0;

    Resolver resolver;
    string err;
    std::vector<string> paths = { "/no/such/file.json" };
    bool ok = resolver.LoadPaths(paths, err);
    if (ok) {
        std::cerr << "FAIL: Resolver.LoadPaths should fail for invalid path" << std::endl;
        failures++;
    } else if (err.empty()) {
        std::cerr << "FAIL: Resolver.LoadPaths should provide an error message for invalid path" << std::endl;
        failures++;
    }

    return failures;
}

static int test_resolver_valid_overlay() {
    int failures = 0;

    // Prepare a minimal overlay file
    const std::string tmpPath = "/tmp/appgateway_resolver_test.json";
    const std::string content = R"JSON(
{
  "resolutions": {
    "device.uid": { "alias": "org.rdk.PluginB.method" }
  }
}
)JSON";

    {
        std::ofstream ofs(tmpPath, std::ios::out | std::ios::trunc);
        if (!ofs.good()) {
            std::cerr << "FAIL: Unable to create temporary file: " << tmpPath << std::endl;
            return failures + 1;
        }
        ofs << content;
    }

    Resolver resolver;
    string err;
    std::vector<string> paths = { tmpPath };
    bool ok = resolver.LoadPaths(paths, err);
    if (!ok) {
        std::cerr << "FAIL: Resolver.LoadPaths should succeed for valid overlay: " << err << std::endl;
        failures++;
    } else {
        Core::JSON::Object out;
        bool got = resolver.Get("anyApp", "device.uid", Core::JSON::Object(), out);
        if (!got) {
            std::cerr << "FAIL: Resolver.Get should return true for device.uid after overlay" << std::endl;
            failures++;
        } else {
            auto v = out.Get(_T("alias"));
            if (v.Content() != Core::JSON::Variant::type::STRING || v.String() != "org.rdk.PluginB.method") {
                std::cerr << "FAIL: Resolver overlay content mismatch. alias=" << v.String() << std::endl;
                failures++;
            }
        }
    }

    // best-effort cleanup
    (void)std::remove(tmpPath.c_str());

    return failures;
}

static int test_request_router_echo() {
    int failures = 0;

    RequestRouter router(nullptr, "");
    Core::JSON::Object reso;
    reso[_T("alias")] = Core::JSON::Variant("org.rdk.PluginX.method");
    Core::JSON::Object params;
    params[_T("k")] = Core::JSON::Variant(42);

    Core::JSON::Object response;
    uint32_t rc = router.DispatchResolved(reso, params, response);
    if (rc != Core::ERROR_NONE) {
        std::cerr << "FAIL: RequestRouter.DispatchResolved should succeed, rc=" << rc << std::endl;
        failures++;
    } else {
        if (response.HasLabel(_T("resolution")) == false || response.HasLabel(_T("params")) == false) {
            std::cerr << "FAIL: RequestRouter echo response missing keys" << std::endl;
            failures++;
        } else {
            auto vr = response.Get(_T("resolution"));
            auto vp = response.Get(_T("params"));
            if (vr.Content() != Core::JSON::Variant::type::OBJECT || vp.Content() != Core::JSON::Variant::type::OBJECT) {
                std::cerr << "FAIL: RequestRouter echo response keys should be objects" << std::endl;
                failures++;
            }
        }
    }

    return failures;
}

int main() {
    int failures = 0;

    failures += test_resolution_store_overlay();
    failures += test_resolver_invalid_path();
    failures += test_resolver_valid_overlay();
    failures += test_request_router_echo();

    if (failures == 0) {
        std::cout << "All AppGateway tests passed." << std::endl;
    } else {
        std::cerr << failures << " AppGateway test(s) failed." << std::endl;
    }

    return failures == 0 ? 0 : 1;
}
