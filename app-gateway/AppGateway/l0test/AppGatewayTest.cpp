#include <iostream>
#include <cassert>

// Ensure Core::Service template is available
#include <core/core.h>

#include "../AppGateway.h"
#include "ServiceMock.h"

using namespace WPEFramework;

int main()
{
    // Create service mock and plugin under test via Core::Service
    L0Test::ServiceMock service;
    PluginHost::IPlugin* plugin = Core::Service<Plugin::AppGateway>::Create<PluginHost::IPlugin>();
    if (plugin == nullptr) {
        std::cerr << "Failed to create AppGateway via Core::Service" << std::endl;
        return 1;
    }

    // Initialize plugin
    const string initResult = plugin->Initialize(&service);
    if (initResult.empty() == false) {
        std::cerr << "Initialize failed: " << initResult << std::endl;
        plugin->Release();
        return 2;
    }

    // Obtain the aggregated resolver interface from the plugin using interface ID
    Exchange::IAppGatewayResolver* resolver = static_cast<Exchange::IAppGatewayResolver*>(
        plugin->QueryInterface(Exchange::IAppGatewayResolver::ID));
    if (resolver == nullptr) {
        std::cerr << "Resolver interface not available from plugin" << std::endl;
        plugin->Deinitialize(&service);
        plugin->Release();
        return 3;
    }

    // Call a minimal Resolve path
    Exchange::GatewayContext ctx;
    ctx.requestId    = 1;
    ctx.connectionId = 1;
    ctx.appId        = "com.example.test";

    string result;
    const Core::hresult rc = resolver->Resolve(ctx, "org.rdk.AppGateway", "dummy.method", "{}", result);

    // Basic asserts
    if (rc != Core::ERROR_NONE) {
        std::cerr << "Resolve returned error: " << rc << std::endl;
        resolver->Release();
        plugin->Deinitialize(&service);
        plugin->Release();
        return 4;
    }

    // Expect our fake resolver to return "null" payload
    if (result != "null") {
        std::cerr << "Unexpected resolution payload: " << result << std::endl;
        resolver->Release();
        plugin->Deinitialize(&service);
        plugin->Release();
        return 5;
    }

    // Release resolver reference obtained from QueryInterface
    resolver->Release();

    // Deinitialize plugin and release the plugin reference
    plugin->Deinitialize(&service);
    plugin->Release();

    std::cout << "AppGateway l0test passed." << std::endl;
    return 0;
}
