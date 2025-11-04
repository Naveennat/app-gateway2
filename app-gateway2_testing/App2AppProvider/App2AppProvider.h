#pragma once

#include <atomic>
#include <memory>
#include <string>
#include "Module.h"
#include "IApp2AppProvider.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class App2AppProviderImplementation; // forward

class App2AppProvider : public PluginHost::IPlugin {
public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

    // PUBLIC_INTERFACE
    const string Initialize(PluginHost::IShell* service) override;
    /** Initialize the plugin and prepare implementation. */

    // PUBLIC_INTERFACE
    void Deinitialize(PluginHost::IShell* service) override;
    /** Deinitialize plugin and free resources. */

    // PUBLIC_INTERFACE
    string Information() const override;
    /** Return plugin information. */

    // PUBLIC_INTERFACE
    uint32_t AddRef() const override;
    // PUBLIC_INTERFACE
    uint32_t Release() const override;
    // PUBLIC_INTERFACE
    void* QueryInterface(const uint32_t id) override;

private:
    Exchange::IAppGateway* AcquireGateway_(PluginHost::IShell* service) const;

private:
    mutable std::atomic<uint32_t> _refCount;
    PluginHost::IShell* _service;

    std::unique_ptr<App2AppProviderImplementation> _impl;
    Exchange::IApp2AppProvider* _iface;
};

} // namespace Plugin
} // namespace WPEFramework
