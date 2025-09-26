#pragma once

// App2AppProvider Thunder plugin: lifecycle and interface exposure.
// This class wires the implementation and exposes Exchange::IApp2AppProvider
// over COMRPC via QueryInterface.

#include <atomic>
#include <memory>
#include <string>
#include "Module.h"

// Public interfaces for provider and gateway
#include "IApp2AppProvider.h"
#include "IAppGateway.h"
#include "UtilsLogging.h"

namespace WPEFramework {
namespace Plugin {

class App2AppProviderImplementation; // Forward declaration

class App2AppProvider : public PluginHost::IPlugin {
public:
    App2AppProvider(const App2AppProvider&) = delete;
    App2AppProvider& operator=(const App2AppProvider&) = delete;

    App2AppProvider();
    ~App2AppProvider() override;

    // PUBLIC_INTERFACE
    const string Initialize(PluginHost::IShell* service) override;
    /**
     * Initialize the App2AppProvider plugin.
     * @param service IShell pointer from Thunder host used to acquire other COMRPC interfaces.
     * @return Empty string on success, otherwise an error text.
     */

    // PUBLIC_INTERFACE
    void Deinitialize(PluginHost::IShell* service) override;
    /**
     * Deinitialize the plugin and free resources.
     * @param service IShell pointer from Thunder host.
     */

    // PUBLIC_INTERFACE
    string Information() const override;
    /**
     * Return plugin meta information (empty for now).
     * @return String with plugin information.
     */

    // IUnknown implementation
    // PUBLIC_INTERFACE
    uint32_t AddRef() const override;
    /** Increase reference count for this plugin instance. */

    // PUBLIC_INTERFACE
    uint32_t Release() const override;
    /** Decrease reference count; deletes object at zero. */

    // PUBLIC_INTERFACE
    void* QueryInterface(const uint32_t id) override;
    /**
     * Query for interfaces exposed by this plugin. Supported:
     * - PluginHost::IPlugin::ID -> this
     * - Exchange::IApp2AppProvider::ID -> implementation pointer
     */

private:
    // Acquire a pointer to AppGateway by callsign from the shell.
    Exchange::IAppGateway* AcquireGateway_(PluginHost::IShell* service) const;

private:
    mutable std::atomic<uint32_t> _refCount;
    PluginHost::IShell* _service;

    // Owned implementation that actually implements IApp2AppProvider.
    std::unique_ptr<App2AppProviderImplementation> _impl;

    // Cached interface pointer exposed through QueryInterface (non-owning; points to _impl)
    Exchange::IApp2AppProvider* _iface;
};

} // namespace Plugin
} // namespace WPEFramework
