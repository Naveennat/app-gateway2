#pragma once

/*
 * CloudStore-style l0test ServiceMock:
 * - Implements PluginHost::IShell so the plugin can Initialize()/Deinitialize().
 * - Implements PluginHost::IShell::ICOMLink so IShell::Root<T>() can instantiate
 *   in-proc fakes via COMLink()->Instantiate(...).
 *
 * Notes:
 * - This mock is aligned to the installed Thunder version found under
 *   dependencies/install/include/WPEFramework/plugins/IShell.h.
 * - We keep behavior deterministic and offline (no filesystem/network access).
 */

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <algorithm>

#include <Module.h>

#include <core/core.h>
#include <plugins/IShell.h>

#include <interfaces/IAppGateway.h>
#include <interfaces/IConfiguration.h>

namespace L0Test {

    using string = std::string;

    // Test-only interface ID to safely obtain the concrete ResponderFake instance via QueryInterface
    // without relying on dynamic_cast across shared-library boundaries.
    static constexpr uint32_t ID_RESPONDER_FAKE = 0xF0F0F001;

    // A simple deterministic resolver fake with multiple error paths.
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
            // No-op for offline tests.
            return WPEFramework::Core::ERROR_NONE;
        }

        // IAppGatewayResolver
        WPEFramework::Core::hresult Configure(IStringIterator* const& /*paths*/) override
        {
            // Accept configuration in tests.
            return WPEFramework::Core::ERROR_NONE;
        }

        WPEFramework::Core::hresult Resolve(const WPEFramework::Exchange::GatewayContext& /*context*/,
                                            const string& /*origin*/,
                                            const string& method,
                                            const string& /*params*/,
                                            string& result) override
        {
            // Deterministic error mapping controlled by method name.
            // This allows l0tests to cover error-path behavior without needing the real
            // AppGatewayImplementation filesystem-driven resolver configuration.
            if (method == "l0.notPermitted") {
                result = "{\"error\":\"NotPermitted\"}";
                return WPEFramework::Core::ERROR_PRIVILIGED_REQUEST;
            }
            if (method == "l0.notSupported") {
                result = "{\"error\":\"NotSupported\"}";
                return WPEFramework::Core::ERROR_NOT_SUPPORTED;
            }
            if (method == "l0.notAvailable") {
                result = "{\"error\":\"NotAvailable\"}";
                return WPEFramework::Core::ERROR_UNAVAILABLE;
            }

            // Success path: return a JSON 'null' resolution (matches current tests).
            result = "null";
            return WPEFramework::Core::ERROR_NONE;
        }

    private:
        mutable std::atomic<uint32_t> _refCount;
    };

    // Responder fake with controllable transport availability and simple notification/context scaffolding.
    class ResponderFake final : public WPEFramework::Exchange::IAppGatewayResponder,
                                public WPEFramework::Exchange::IConfiguration {
    public:
        explicit ResponderFake(const bool transportEnabled = true)
            : _refCount(1)
            , _transportEnabled(transportEnabled)
        {
        }

        ~ResponderFake() override
        {
            // Release all registered notifications
            for (auto* n : _notifications) {
                if (n != nullptr) {
                    n->Release();
                }
            }
            _notifications.clear();
        }

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
            // L0 test helper: allow callers to retrieve the concrete fake type safely.
            if (id == ID_RESPONDER_FAKE) {
                AddRef();
                return this;
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
            return _transportEnabled ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::hresult Emit(const WPEFramework::Exchange::GatewayContext& /*context*/,
                                         const string& /*method*/,
                                         const string& /*payload*/) override
        {
            return _transportEnabled ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::hresult Request(const uint32_t /*connectionId*/,
                                            const uint32_t /*id*/,
                                            const string& /*method*/,
                                            const string& /*params*/) override
        {
            return _transportEnabled ? WPEFramework::Core::ERROR_NONE : WPEFramework::Core::ERROR_UNAVAILABLE;
        }

        WPEFramework::Core::hresult GetGatewayConnectionContext(const uint32_t connectionId,
                                                                const string& contextKey,
                                                                string& contextValue) override
        {
            std::lock_guard<std::mutex> lock(_ctxMutex);
            auto itConn = _contexts.find(connectionId);
            if (itConn == _contexts.end()) {
                return WPEFramework::Core::ERROR_BAD_REQUEST;
            }
            auto itKey = itConn->second.find(contextKey);
            if (itKey == itConn->second.end()) {
                return WPEFramework::Core::ERROR_BAD_REQUEST;
            }
            contextValue = itKey->second;
            return WPEFramework::Core::ERROR_NONE;
        }

        WPEFramework::Core::hresult Register(INotification* notification) override
        {
            if (notification == nullptr) {
                return WPEFramework::Core::ERROR_GENERAL;
            }
            // Avoid duplicate registrations
            if (std::find(_notifications.begin(), _notifications.end(), notification) == _notifications.end()) {
                _notifications.push_back(notification);
                notification->AddRef();
            }
            return WPEFramework::Core::ERROR_NONE;
        }

        WPEFramework::Core::hresult Unregister(INotification* notification) override
        {
            if (notification == nullptr) {
                return WPEFramework::Core::ERROR_GENERAL;
            }
            auto it = std::find(_notifications.begin(), _notifications.end(), notification);
            if (it != _notifications.end()) {
                (*it)->Release();
                _notifications.erase(it);
                return WPEFramework::Core::ERROR_NONE;
            }
            // Not found; keep behavior tolerant
            return WPEFramework::Core::ERROR_NONE;
        }

        // Helpers for tests (not part of Exchange interface)
        void SetTransportEnabled(const bool enabled) { _transportEnabled = enabled; }

        void SetConnectionContext(const uint32_t connectionId, const string& key, const string& value)
        {
            std::lock_guard<std::mutex> lock(_ctxMutex);
            _contexts[connectionId][key] = value;
        }

        void SimulateAppConnectionChanged(const string& appId, const uint32_t connectionId, const bool connected)
        {
            for (auto* n : _notifications) {
                if (n != nullptr) {
                    n->OnAppConnectionChanged(appId, connectionId, connected);
                }
            }
        }

    private:
        mutable std::atomic<uint32_t> _refCount;
        bool _transportEnabled;
        std::list<INotification*> _notifications;

        std::mutex _ctxMutex;
        std::unordered_map<uint32_t, std::unordered_map<string, string>> _contexts;
    };

    class ServiceMock final : public WPEFramework::PluginHost::IShell,
                              public WPEFramework::PluginHost::IShell::ICOMLink {
    public:
        struct Config {
            bool provideResolver;
            bool provideResponder;
            bool responderTransportAvailable;

            // PUBLIC_INTERFACE
            explicit Config(const bool resolver = true, const bool responder = true, const bool responderTransport = true)
                : provideResolver(resolver)
                , provideResponder(responder)
                , responderTransportAvailable(responderTransport)
            {
            }
        };

        explicit ServiceMock(Config cfg = Config(), const bool selfDelete = false)
            : _refCount(1)
            , _instantiateCount(0)
            , _callsign("org.rdk.AppGateway")
            , _className("AppGateway")
            // Always provide responder for L0 success
            , _cfg(cfg)
            , _selfDelete(selfDelete)
        {
            _cfg.provideResponder = true;
            _cfg.provideResolver = true; // Also ensure resolver provided
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
                // L0 test safety:
                // Some tests use stack-allocated ServiceMock instances, but plugin code may AddRef/Release
                // them (e.g., AppGatewayImplementation stores IShell* and releases it in destructor).
                // Deleting a stack object causes heap corruption / SIGABRT.
                //
                // Default behavior in this repo's L0 tests is therefore: do NOT delete on Release(0).
                if (_selfDelete) {
                    delete this;
                }
                return WPEFramework::Core::ERROR_DESTRUCTION_SUCCEEDED;
            }
            return WPEFramework::Core::ERROR_NONE;
        }

        void* QueryInterface(const uint32_t id) override
        {
            if (id == WPEFramework::PluginHost::IShell::ID) {
                AddRef();
                return static_cast<WPEFramework::PluginHost::IShell*>(this);
            }
            return nullptr;
        }

        // IShell
        void EnableWebServer(const string& /*URLPath*/, const string& /*fileSystemPath*/) override {}
        void DisableWebServer() override {}

        string Model() const override { return "l0test-device"; }
        bool Background() const override { return false; }
        string Accessor() const override { return "127.0.0.1:9998"; }
        string WebPrefix() const override { return "/jsonrpc"; }
        string Locator() const override
        {
            // Provide a real locator for PluginHost::IShell::Root() so Thunder can LoadLibrary().
            // The runner script sets APPGATEWAY_PLUGIN_SO to the absolute path of libWPEFrameworkAppGateway.so.
            const char* soPath = std::getenv("APPGATEWAY_PLUGIN_SO");
            if (soPath != nullptr && *soPath != '\0') {
                return string(soPath);
            }

            // Fallback: a reasonable default name (may still resolve via Thunder search paths).
            return "libWPEFrameworkAppGateway.so";
        }
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
            // L0 HARNESS NOTE:
            // Plugin::Config::RootConfig (Thunder SDK) reads IShell::ConfigLine() using RootObject
            // and then (optionally) parses a nested JSON string from the "root" field.
            //
            // In this repo's isolated L0 harness, that nested parsing path has been observed to fail
            // with errors like:
            //   Invalid value. "null" or "{" expected. At character 1: "
            // which prevents IShell::Root<T>() from proceeding and ultimately causes AppGateway
            // Initialize() to fail and JSON-RPC resolve to return ERROR_UNKNOWN_METHOD (53).
            //
            // For L0 tests we do not need any root/out-of-process configuration. Returning a minimal
            // JSON object avoids the nested parse and still allows Root<T>() to route to COMLink()
            // and instantiate our in-proc fakes deterministically.
            return "{}";
        }
        WPEFramework::Core::hresult ConfigLine(const string& /*config*/) override { return WPEFramework::Core::ERROR_NONE; }

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

        void* QueryInterfaceByCallsign(const uint32_t /*id*/, const string& /*name*/) override
        {
            // Not used by current l0tests; return nullptr for all.
            return nullptr;
        }

        WPEFramework::Core::hresult Activate(const reason /*why*/) override { return WPEFramework::Core::ERROR_NONE; }
        WPEFramework::Core::hresult Deactivate(const reason /*why*/) override { return WPEFramework::Core::ERROR_NONE; }
        WPEFramework::Core::hresult Unavailable(const reason /*why*/) override { return WPEFramework::Core::ERROR_NONE; }
        WPEFramework::Core::hresult Hibernate(const uint32_t /*timeout*/) override { return WPEFramework::Core::ERROR_NONE; }
        reason Reason() const override { return reason::REQUESTED; }

        uint32_t Submit(const uint32_t /*Id*/, const WPEFramework::Core::ProxyType<WPEFramework::Core::JSON::IElement>& /*response*/) override
        {
            return WPEFramework::Core::ERROR_NONE;
        }

        WPEFramework::PluginHost::IShell::ICOMLink* COMLink() override
        {
            // Root<T>() will route to COMLink()->Instantiate(...) in main process context.
            return this;
        }

        // ICOMLink
        void Register(WPEFramework::RPC::IRemoteConnection::INotification* /*sink*/) override {}
        void Unregister(const WPEFramework::RPC::IRemoteConnection::INotification* /*sink*/) override {}

        void Register(WPEFramework::PluginHost::IShell::ICOMLink::INotification* /*sink*/) override {}
        void Unregister(WPEFramework::PluginHost::IShell::ICOMLink::INotification* /*sink*/) override {}

        WPEFramework::RPC::IRemoteConnection* RemoteConnection(const uint32_t /*connectionId*/) override
        {
            // In-proc l0tests do not create remote processes.
            return nullptr;
        }

        void* Instantiate(const WPEFramework::RPC::Object& object, const uint32_t /*waitTime*/, uint32_t& connectionId) override
        {
            // AppGateway::Initialize() requests (via IShell::Root):
            //  - "AppGatewayImplementation"          (Exchange::IAppGatewayResolver)
            //  - "AppGatewayResponderImplementation" (Exchange::IAppGatewayResponder)
            //
            // In some Thunder builds, Object::ClassName() can be fully-qualified
            // (e.g., "WPEFramework::Plugin::AppGatewayImplementation"). If we only match the short
            // name, our mock would fail to intercept instantiation and Thunder may fall back to
            // creating the real implementation (which can trigger COM-RPC wiring and crash in
            // this isolated L0 environment).
            //
            // So we match by suffix to ensure our deterministic fakes are always used in-proc.
            connectionId = 1;
            _instantiateCount.fetch_add(1, std::memory_order_acq_rel);

            const std::string className = object.ClassName();

            auto endsWith = [](const std::string& s, const std::string& suffix) -> bool {
                if (s.size() < suffix.size()) {
                    return false;
                }
                return s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
            };

            // Resolver: accept both implementation and interface-style names to be robust across Thunder variants.
            // Some SDKs use short names like "AppGatewayResolver" during Root<T>() instantiation.
            if (endsWith(className, "AppGatewayImplementation") || endsWith(className, "AppGatewayResolver")) {
                return (_cfg.provideResolver ? static_cast<WPEFramework::Exchange::IAppGatewayResolver*>(new ResolverFake()) : nullptr);
            }

            // Responder: similarly accept "AppGatewayResponder" in addition to implementation name.
            if (endsWith(className, "AppGatewayResponderImplementation") || endsWith(className, "AppGatewayResponder")) {
                return (_cfg.provideResponder ? static_cast<WPEFramework::Exchange::IAppGatewayResponder*>(new ResponderFake(_cfg.responderTransportAvailable)) : nullptr);
            }

            return nullptr;
        }

        // Helpers for tests
        void Callsign(const std::string& cs) { _callsign = cs; }
        void ClassName(const std::string& cn) { _className = cn; }

    private:
        mutable std::atomic<uint32_t> _refCount;
        std::atomic<uint32_t> _instantiateCount;
        std::string _callsign;
        std::string _className;
        Config _cfg;
        const bool _selfDelete;
    };

} // namespace L0Test
