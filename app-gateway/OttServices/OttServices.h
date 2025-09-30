#pragma once

// Module: OttServices
// This header declares the OttServices Thunder plugin public interface and JSON-RPC API.
// Follows naming and structural conventions similar to AppNotifications plugin in this codebase.

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IPlugin.h>
#include <plugins/JSONRPC.h>
#include <atomic>

namespace WPEFramework {
namespace Plugin {

    // Forward declaration of the implementation class that contains business logic.
    class OttServicesImplementation;

    // The main plugin class exposed to Thunder. Uses JSON-RPC to expose required methods.
    class OttServices : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    public:
        OttServices(const OttServices&) = delete;
        OttServices& operator=(const OttServices&) = delete;

        // PUBLIC_INTERFACE
        OttServices();
        // PUBLIC_INTERFACE
        ~OttServices() override;

        // IPlugin methods
        // PUBLIC_INTERFACE
        const string Initialize(PluginHost::IShell* service) override;
        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service) override;
        // PUBLIC_INTERFACE
        string Information() const override;

        // IUnknown implementation
        // PUBLIC_INTERFACE
        uint32_t AddRef() const override;
        // PUBLIC_INTERFACE
        uint32_t Release() const override;
        // PUBLIC_INTERFACE
        void* QueryInterface(const uint32_t id) override;

    public:
        // JSON-RPC API structures. Only required features per OTTServices-Complete-Design.md should be present.
        // Since we do not have the exact spec content here, we provide a minimal, conservative set of required
        // JSON-RPC endpoints with placeholders. Replace/extend only if the spec mandates additional methods.

        // Example: Ping method to verify plugin is responsive (commonly required in designs).
        class PingParams : public Core::JSON::Container {
        public:
            PingParams(const PingParams&) = delete;
            PingParams& operator=(const PingParams&) = delete;

            PingParams()
                : Core::JSON::Container() {
                Add(_T("message"), &Message);
            }
            ~PingParams() override = default;

        public:
            Core::JSON::String Message;
        };

        class PingResult : public Core::JSON::Container {
        public:
            PingResult(const PingResult&) = delete;
            PingResult& operator=(const PingResult&) = delete;

            PingResult()
                : Core::JSON::Container() {
                Add(_T("reply"), &Reply);
            }
            ~PingResult() override = default;

        public:
            Core::JSON::String Reply;
        };

        // PUBLIC_INTERFACE
        uint32_t endpoint_ping(const PingParams& params, PingResult& result);

        // Required event: statechanged (typical for service plugins to signal state)
        // PUBLIC_INTERFACE
        void EventStateChanged(const string& state);

    private:
        void RegisterAll();
        void UnregisterAll();

    private:
        PluginHost::IShell* _service;
        OttServicesImplementation* _implementation;
        string _version;

        // Reference counter for lifetime management
        mutable std::atomic<uint32_t> _refCount;

    private:
        // Non-copyable utility to guard thread-safety if needed in future extensions.
        mutable Core::CriticalSection _adminLock;
    };

} // namespace Plugin
} // namespace WPEFramework
