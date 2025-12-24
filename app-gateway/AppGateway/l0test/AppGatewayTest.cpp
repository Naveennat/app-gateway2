#include <iostream>
#include <string>

#include <plugins/IDispatcher.h>
#include <core/core.h>

#include "../AppGateway.h"
#include "ServiceMock.h"

using WPEFramework::Core::ERROR_BAD_REQUEST;
using WPEFramework::Core::ERROR_NONE;
using WPEFramework::Plugin::AppGateway;
using WPEFramework::PluginHost::IDispatcher;
using WPEFramework::PluginHost::IPlugin;

static int test_Init_ProvidesDispatcher(IPlugin* plugin, L0Test::ServiceMock* service) {
    const string initResult = plugin->Initialize(service);
    if (!initResult.empty()) {
        std::cerr << "Initialize failed: " << initResult << std::endl;
        return 1;
    }

    auto dispatcher = plugin->QueryInterface<IDispatcher>();
    if (dispatcher == nullptr) {
        std::cerr << "IDispatcher not available" << std::endl;
        plugin->Deinitialize(service);
        return 2;
    }
    dispatcher->Release();

    plugin->Deinitialize(service);
    return 0;
}

static int test_DirectResolverPath(IPlugin* plugin, L0Test::ServiceMock* service) {
    const string initResult = plugin->Initialize(service);
    if (!initResult.empty()) {
        std::cerr << "Initialize failed: " << initResult << std::endl;
        return 1;
    }

    auto* resolver = static_cast<WPEFramework::Exchange::IAppGatewayResolver*>(
        plugin->QueryInterface(WPEFramework::Exchange::IAppGatewayResolver::ID));
    if (resolver == nullptr) {
        std::cerr << "Resolver interface not available" << std::endl;
        plugin->Deinitialize(service);
        return 2;
    }

    WPEFramework::Exchange::GatewayContext ctx;
    ctx.requestId = 42;
    ctx.connectionId = 7;
    ctx.appId = "com.example.test";

    string resolution;
    const auto rc = resolver->Resolve(ctx, "org.rdk.AppGateway", "dummy.method", "{}", resolution);
    if (rc != ERROR_NONE) {
        std::cerr << "Resolve() returned error: " << rc << std::endl;
        resolver->Release();
        plugin->Deinitialize(service);
        return 3;
    }
    if (resolution != "null") {
        std::cerr << "Unexpected resolution payload: " << resolution << std::endl;
        resolver->Release();
        plugin->Deinitialize(service);
        return 4;
    }

    resolver->Release();
    plugin->Deinitialize(service);
    return 0;
}

static int test_JsonRpcResolve_OK(IPlugin* plugin, L0Test::ServiceMock* service) {
    const string initResult = plugin->Initialize(service);
    if (!initResult.empty()) {
        std::cerr << "Initialize failed: " << initResult << std::endl;
        return 1;
    }

    auto dispatcher = plugin->QueryInterface<IDispatcher>();
    if (dispatcher == nullptr) {
        std::cerr << "IDispatcher not available" << std::endl;
        plugin->Deinitialize(service);
        return 2;
    }

    const string paramsJson = R"({
        "requestId": 1001,
        "connectionId": 10,
        "appId": "com.example.test",
        "origin": "org.rdk.AppGateway",
        "method": "dummy.method",
        "params": "{}"
    })";

    string jsonResponse;
    const auto rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);

    dispatcher->Release();
    plugin->Deinitialize(service);

    if (rc != ERROR_NONE) {
        std::cerr << "JSON-RPC resolve returned error: " << rc << std::endl;
        return 3;
    }
    if (jsonResponse != "null") {
        std::cerr << "Unexpected JSON-RPC resolve response: " << jsonResponse << std::endl;
        return 4;
    }

    return 0;
}

static int test_JsonRpcResolve_BadRequest(IPlugin* plugin, L0Test::ServiceMock* service) {
    const string initResult = plugin->Initialize(service);
    if (!initResult.empty()) {
        std::cerr << "Initialize failed: " << initResult << std::endl;
        return 1;
    }

    auto dispatcher = plugin->QueryInterface<IDispatcher>();
    if (dispatcher == nullptr) {
        std::cerr << "IDispatcher not available" << std::endl;
        plugin->Deinitialize(service);
        return 2;
    }

    const string badParams = R"({ "appId": "missing_required_fields" })";
    string jsonResponse;
    const auto rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", badParams, jsonResponse);

    dispatcher->Release();
    plugin->Deinitialize(service);

    if (rc != ERROR_BAD_REQUEST) {
        std::cerr << "Expected ERROR_BAD_REQUEST but got: " << rc << std::endl;
        return 3;
    }

    return 0;
}

int main() {
    // Create service mock and plugin under test via Core::Service
    auto* service = new L0Test::ServiceMock();
    auto* plugin  = WPEFramework::Core::Service<AppGateway>::Create<IPlugin>();
    if (plugin == nullptr || service == nullptr) {
        std::cerr << "Failed to create service or plugin" << std::endl;
        if (plugin) plugin->Release();
        if (service) service->Release();
        return 1;
    }

    int failures = 0;
    failures += test_Init_ProvidesDispatcher(plugin, service);
    failures += test_DirectResolverPath(plugin, service);
    failures += test_JsonRpcResolve_OK(plugin, service);
    failures += test_JsonRpcResolve_BadRequest(plugin, service);

    plugin->Release();
    service->Release();

    if (failures == 0) {
        std::cout << "AppGateway l0test passed." << std::endl;
        return 0;
    }

    std::cerr << "AppGateway l0test failures: " << failures << std::endl;
    return failures;
}
