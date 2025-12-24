#pragma once

/*
 * Minimal ServiceMock for l0test:
 * - Implements PluginHost::IShell to serve as the Thunder service for the plugin.
 * - Implements PluginHost::IShell::ICOMLink so that Shell::Root() can instantiate
 *   test doubles for AppGateway resolver/responder via COMLink()->Instantiate.
 *
 * This mock is intentionally lightweight and only implements the methods exercised
 * by AppGateway during Initialize()/Deinitialize() and by the basic test path.
 */

#include <atomic>
#include <string>
#include <vector>
#include <memory>

#include "../Module.h"
#include <plugins/IShell.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IAppGateway.h>

namespace L0Test {

// Minimal IAppGatewayResolver test double
class ResolverFake final : public WPEFramework::Exchange::IAppGatewayResolver,
                           public WPEFramework::Exchange::IConfiguration {
public:
    ResolverFake()
        : _refCount(1)
    {
    }

    ~ResolverFake() override = default;

    // Core::IUnknown
    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (newCount == 0) {
            delete this;
            // Return value matches Thunder "destruction succeeded" contract
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppGatewayResolver::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppGatewayResolver*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IConfiguration
    uint32_t Configure(WPEFramework::PluginHost::IShell* /*shell*/) override
    {
        // Minimal configure success
        return WPEFramework::Core::ERROR_NONE;
    }

    // IAppGatewayResolver
    WPEFramework::Core::hresult Configure(IStringIterator* const& /*paths*/) override
    {
        // Minimal success: accept configuration
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Resolve(const WPEFramework::Exchange::GatewayContext& /*context*/,
                                        const string& /*origin*/,
                                        const string& /*method*/,
                                        const string& /*params*/,
                                        string& result) override
    {
        // Minimal successful resolve returning an empty (null) resolution
        result = "null";
        return WPEFramework::Core::ERROR_NONE;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
};

// Minimal IAppGatewayResponder test double
class ResponderFake final : public WPEFramework::Exchange::IAppGatewayResponder,
                            public WPEFramework::Exchange::IConfiguration {
public:
    ResponderFake()
        : _refCount(1)
    {
    }

    ~ResponderFake() override = default;

    // Core::IUnknown
    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (newCount == 0) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t id) override
    {
        if (id == WPEFramework::Exchange::IAppGatewayResponder::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IAppGatewayResponder*>(this);
        }
        if (id == WPEFramework::Exchange::IConfiguration::ID) {
            AddRef();
            return static_cast<WPEFramework::Exchange::IConfiguration*>(this);
        }
        return nullptr;
    }

    // IConfiguration
    uint32_t Configure(WPEFramework::PluginHost::IShell* /*shell*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // IAppGatewayResponder
    WPEFramework::Core::hresult Respond(const WPEFramework::Exchange::GatewayContext& /*context*/,
                                        const string& /*payload*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Emit(const WPEFramework::Exchange::GatewayContext& /*context*/,
                                     const string& /*method*/,
                                     const string& /*payload*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Request(const uint32_t /*connectionId*/,
                                        const uint32_t /*id*/,
                                        const string& /*method*/,
                                        const string& /*params*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult GetGatewayConnectionContext(const uint32_t /*connectionId*/,
                                                            const string& /*contextKey*/,
                                                            string& /*contextValue*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Register(INotification* /*notification*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Unregister(INotification* /*notification*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

private:
    mutable std::atomic<uint32_t> _refCount;
};

// Lightweight IShell + ICOMLink combined mock
class ServiceMock final : public WPEFramework::PluginHost::IShell,
                          public WPEFramework::PluginHost::IShell::ICOMLink {
public:
    ServiceMock()
        : _refCount(1)
        , _instantiateCount(0)
        , _callsign("org.rdk.AppGateway")
        , _className("AppGateway")
    {
    }

    ~ServiceMock() override = default;

    // Core::IUnknown
    uint32_t AddRef() const override
    {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t Release() const override
    {
        const uint32_t newCount = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (newCount == 0) {
            delete this;
            return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
        }
        return WPEFramework::Core::ERROR_NONE;
    }

    void* QueryInterface(const uint32_t /*id*/) override
    {
        // Not used by the test
        return nullptr;
    }

    // IShell (only methods needed by the plugin/test are implemented meaningfully)
    void EnableWebServer(const string& /*URLPath*/, const string& /*fileSystemPath*/) override {}
    void DisableWebServer() override {}

    string Model() const override { return "l0test-device"; }
    bool Background() const override { return false; }
    string Accessor() const override { return "127.0.0.1:9998"; }
    string WebPrefix() const override { return "/jsonrpc"; }
    string Locator() const override { return "WPEFrameworkAppGateway"; }
    string ClassName() const override { return _className; }
    string Versions() const override { return "1.0.0"; }
    string Callsign() const override { return _callsign; }

    string PersistentPath() const override { return "/tmp"; }
    string VolatilePath() const override { return "/tmp"; }
    string DataPath() const override { return "/tmp"; }
    string ProxyStubPath() const override { return "/tmp"; }
    string SystemPath() const override { return "/tmp"; }
    string PluginPath() const override { return "/tmp"; }
    string SystemRootPath() const override { return "/"; }

    WPEFramework::Core::hresult SystemRootPath(const string& /*systemRootPath*/) override { return WPEFramework::Core::ERROR_NONE; }

    startup Startup() const override { return startup::ACTIVATED; }
    WPEFramework::Core::hresult Startup(const startup /*value*/) override { return WPEFramework::Core::ERROR_NONE; }

    string Substitute(const string& input) const override { return input; }

    bool Resumed() const override { return false; }
    WPEFramework::Core::hresult Resumed(const bool /*value*/) override { return WPEFramework::Core::ERROR_NONE; }

    string HashKey() const override { return "hash"; }

    string ConfigLine() const override
    {
        // Provide an empty/minimal config JSON string
        return "{}";
    }
    WPEFramework::Core::hresult ConfigLine(const string& /*config*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    WPEFramework::Core::hresult Metadata(string& info /* @out */) const override
    {
        info = R"({"name":"AppGateway","version":"1.0.0"})";
        return WPEFramework::Core::ERROR_NONE;
    }

    bool IsSupported(const uint8_t /*version*/) const override { return true; }

    WPEFramework::PluginHost::ISubSystem* SubSystems() override { return nullptr; }

    void Notify(const string& /*message*/) override {}

    void Register(WPEFramework::PluginHost::IPlugin::INotification* /*sink*/) override {}
    void Unregister(WPEFramework::PluginHost::IPlugin::INotification* /*sink*/) override {}

    state State() const override { return state::ACTIVATED; }

    void* /* @interface:id */ QueryInterfaceByCallsign(const uint32_t /*id*/, const string& /*name*/) override
    {
        // Not used in this minimal l0test
        return nullptr;
    }

    WPEFramework::Core::hresult Activate(const reason /*why*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Deactivate(const reason /*why*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Unavailable(const reason /*why*/) override { return WPEFramework::Core::ERROR_NONE; }
    WPEFramework::Core::hresult Hibernate(const uint32_t /*timeout*/) override { return WPEFramework::Core::ERROR_NONE; }
    reason Reason() const override { return reason::REQUESTED; }

    // JSON submit (not used)
    uint32_t Submit(const uint32_t /*Id*/, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>& /*response*/) override
    {
        return WPEFramework::Core::ERROR_NONE;
    }

    // Provide ICOMLink handle
    WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
    {
        // In main process, Root() routes to COMLink()->Instantiate(...). Return self as the handler.
        return this;
    }

    // ICOMLink
    void Register(WPEFramework::RPC::IRemoteConnection::INotification* /*sink*/) override {}
    void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification* /*sink*/) override {}

    void Register(WPEFramework::PluginHost::IShell::ICOMLink::INotification* /*sink*/) override {}
    void Unregister(WPEFramework::PluginHost::IShell::ICOMLink::INotification* /*sink*/) override {}

    WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t /*connectionId*/) override
    {
        // Our fakes are in-proc only; return nullptr (no remote connection to terminate)
        return nullptr;
    }

    void* Instantiate(const WPEFramework::RPC::Object& /*object*/, const uint32_t /*waitTime*/, uint32_t& connectionId) override
    {
        // AppGateway calls Root<...>(..., "AppGatewayImplementation") and "AppGatewayResponderImplementation".
        // We don't inspect RPC::Object here; return resolver on first call, responder on second call.
        connectionId = 1;

        if (_instantiateCount == 0) {
            _instantiateCount++;
            return static_cast<WPEFramework::Exchange::IAppGatewayResolver*>(new ResolverFake());
        } else if (_instantiateCount == 1) {
            _instantiateCount++;
            return static_cast<WPEFramework::Exchange::IAppGatewayResponder*>(new ResponderFake());
        }

        // No further instantiations required for this minimal test
        return nullptr;
    }

    // Helpers for test configuration
    void Callsign(const std::string& cs) { _callsign = cs; }
    void ClassName(const std::string& cn) { _className = cn; }

private:
    mutable std::atomic<uint32_t> _refCount;
    std::atomic<uint32_t> _instantiateCount;
    std::string _callsign;
    std::string _className;
};

} // namespace L0Test
