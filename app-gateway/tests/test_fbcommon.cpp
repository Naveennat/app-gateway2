#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include "AppGateway/Resolver.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;

static bool WriteTempFile(const std::string& path, const std::string& content) {
    std::ofstream ofs(path, std::ios::out | std::ios::trunc);
    if (!ofs.is_open()) return false;
    ofs << content;
    return true;
}

int main() {
    int failures = 0;

    // Prepare a minimal FbCommon resolution overlay
    const std::string tmpPath = "/tmp/fbcommon_resolver_test.json";
    const std::string content = R"JSON(
{
  "resolutions": {
    "device.audio": { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.hdcp":  { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.hdr":   { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.screenResolution": { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.videoResolution":  { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.sku":   { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.make":  { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.name":  { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.setName": { "alias": "org.rdk.FbCommon", "useComRpc": true },
    "device.onScreenResolutionChanged": {
      "alias": "org.rdk.DisplaySettings",
      "event": "org.rdk.DisplaySettings.resolutionChanged"
    },
    "device.onHdcpChanged": {
      "alias": "org.rdk.HdcpProfile",
      "event": "org.rdk.HdcpProfile.onDisplayConnectionChanged"
    }
  }
}
)JSON";

    if (!WriteTempFile(tmpPath, content)) {
        std::cerr << "FAIL: Unable to write temp file: " << tmpPath << std::endl;
        return 1;
    }

    Resolver resolver(nullptr, tmpPath);
    if (!resolver.IsConfigured()) {
        std::cerr << "FAIL: Resolver should be configured after loading overlay\n";
        failures++;
    }

    auto checkAlias = [&](const std::string& key, const std::string& expectedAlias, bool expectComRpc) {
        std::string alias = resolver.ResolveAlias(key);
        if (alias != expectedAlias) {
            std::cerr << "FAIL: Alias mismatch for " << key << " -> " << alias << " (expected " << expectedAlias << ")\n";
            failures++;
        }
        bool comrpc = resolver.HasComRpcRequestSupport(key);
        if (comrpc != expectComRpc) {
            std::cerr << "FAIL: useComRpc mismatch for " << key << " -> " << (comrpc?"true":"false") << "\n";
            failures++;
        }
    };

    checkAlias("device.audio", "org.rdk.FbCommon", true);
    checkAlias("device.hdcp", "org.rdk.FbCommon", true);
    checkAlias("device.hdr", "org.rdk.FbCommon", true);
    checkAlias("device.screenResolution", "org.rdk.FbCommon", true);
    checkAlias("device.videoResolution", "org.rdk.FbCommon", true);
    checkAlias("device.sku", "org.rdk.FbCommon", true);
    checkAlias("device.make", "org.rdk.FbCommon", true);
    checkAlias("device.name", "org.rdk.FbCommon", true);
    checkAlias("device.setName", "org.rdk.FbCommon", true);

    if (!resolver.HasEvent("device.onScreenResolutionChanged")) {
        std::cerr << "FAIL: Expected event mapping for device.onScreenResolutionChanged\n";
        failures++;
    }
    if (!resolver.HasEvent("device.onHdcpChanged")) {
        std::cerr << "FAIL: Expected event mapping for device.onHdcpChanged\n";
        failures++;
    }

    if (failures == 0) {
        std::cout << "FbCommon resolver tests passed.\n";
    } else {
        std::cerr << failures << " FbCommon resolver test(s) failed.\n";
    }

    return failures == 0 ? 0 : 1;
}
