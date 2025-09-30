#pragma once

// Module: OttServices
// This header declares the OttServices Thunder plugin public interface and JSON-RPC API.
// Follows naming and structural conventions similar to AppNotifications plugin in this codebase.

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IPlugin.h>
#include <plugins/JSONRPC.h>
#include <atomic>
#include <type_traits>
#include <utility>

#include "IOttServices.h"

namespace WPEFramework {
namespace Plugin {

    // Forward declaration of the implementation class that contains business logic.
    class OttServicesImplementation;

    // The main plugin class exposed to Thunder. Uses JSON-RPC to expose required methods.
    // Also exposes a COMRPC interface (Exchange::IOttServices) through QueryInterface.
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
        auto AddRef() const -> decltype(std::declval<const Core::IReferenceCounted&>().AddRef()) override;
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

        // IOttPermission-aligned JSON-RPC: getpermissions
        class GetPermissionsParams : public Core::JSON::Container {
        public:
            GetPermissionsParams() {
                Add(_T("appId"), &AppId);
            }
            Core::JSON::String AppId;
        };
        class GetPermissionsResult : public Core::JSON::Container {
        public:
            GetPermissionsResult() {
                Add(_T("permissions"), &Permissions);
            }
            Core::JSON::ArrayType<Core::JSON::String> Permissions;
        };
        // PUBLIC_INTERFACE
        uint32_t endpoint_getpermissions(const GetPermissionsParams& params, GetPermissionsResult& result);

        // IOttPermission-aligned JSON-RPC: invalidatepermissions
        class InvalidatePermissionsParams : public Core::JSON::Container {
        public:
            InvalidatePermissionsParams() {
                Add(_T("appId"), &AppId);
            }
            Core::JSON::String AppId;
        };
        // PUBLIC_INTERFACE
        uint32_t endpoint_invalidatepermissions(const InvalidatePermissionsParams& params, Core::JSON::Container& response);

        // IOttPermission-aligned JSON-RPC: updatepermissionscache
        class UpdatePermissionsCacheParams : public Core::JSON::Container {
        public:
            UpdatePermissionsCacheParams() {
                Add(_T("appId"), &AppId);
            }
            Core::JSON::String AppId;
        };
        class UpdatePermissionsCacheResult : public Core::JSON::Container {
        public:
            UpdatePermissionsCacheResult() {
                Add(_T("updated"), &Updated);
                Add(_T("count"), &Count);
            }
            Core::JSON::Boolean Updated;
            Core::JSON::DecUInt32 Count;
        };
        // PUBLIC_INTERFACE
        uint32_t endpoint_updatepermissionscache(const UpdatePermissionsCacheParams& params, UpdatePermissionsCacheResult& result);

        // Required event: statechanged (typical for service plugins to signal state)
        // PUBLIC_INTERFACE
        void EventStateChanged(const string& state);

    private:
        void RegisterAll();
        void UnregisterAll();

    private:
        PluginHost::IShell* _service;
        OttServicesImplementation* _implementation;
        Exchange::IOttServices* _interface; // COMRPC interface pointer exposed via QueryInterface
        string _version;

        // Reference counter for lifetime management
        mutable std::atomic<uint32_t> _refCount;

    private:
        // Non-copyable utility to guard thread-safety if needed in future extensions.
        mutable Core::CriticalSection _adminLock;
    };

} // namespace Plugin
} // namespace WPEFramework
