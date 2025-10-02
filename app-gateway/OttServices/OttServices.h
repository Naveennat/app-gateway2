#pragma once

// Module: OttServices
// This header declares the OttServices Thunder plugin public interface and JSON-RPC API.
// Follows naming and structural conventions similar to AppNotifications plugin in this codebase.

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IPlugin.h>
#include <plugins/JSONRPC.h>
#include <plugins/IShell.h>
#include <atomic>
#include <type_traits>
#include <utility>

#include "IOttServices.h"

namespace WPEFramework {
namespace Plugin {

    // Forward declaration of the implementation class that contains business logic.
    class OttServicesImplementation;

    // The main plugin class exposed to Thunder. Uses JSON-RPC to expose required methods.
    // Also exposes a COMRPC interface (Exchange::IOttServices) through interface aggregation.
    class OttServices : public PluginHost::IPlugin, public PluginHost::JSONRPC {
    private:
        // Remote connection deactivation listener (for parity with AppNotifications/SharedStorage patterns).
        class RemoteConnectionNotification : public RPC::IRemoteConnection::INotification {
        private:
            RemoteConnectionNotification() = delete;
            RemoteConnectionNotification(const RemoteConnectionNotification&) = delete;
            RemoteConnectionNotification& operator=(const RemoteConnectionNotification&) = delete;

        public:
            explicit RemoteConnectionNotification(OttServices& parent)
                : _parent(parent) {
            }
            ~RemoteConnectionNotification() override = default;

            BEGIN_INTERFACE_MAP(RemoteConnectionNotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
            END_INTERFACE_MAP

            void Activated(RPC::IRemoteConnection*) override {
                // Not used
            }
            void Deactivated(RPC::IRemoteConnection* connection) override {
                _parent.Deactivated(connection);
            }

        private:
            OttServices& _parent;
        };

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
        /** Initialize the plugin, set up JSON-RPC, create the implementation and aggregate COM interface.
         * @param service Plugin host shell provided by Thunder
         * @return Empty string on success, otherwise a human readable error.
         */

        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service) override;
        /** Deinitialize the plugin, unregister notifications and free resources. */

        // PUBLIC_INTERFACE
        string Information() const override;
        /** Return plugin information string (name/version/category). */

        // Declare support for IPlugin, JSONRPC/IDispatcher and the aggregated COM interface.
        BEGIN_INTERFACE_MAP(OttServices)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IOttServices, _interface)
        END_INTERFACE_MAP

    public:
        // JSON-RPC API structures. Only required features per OTTServices-Complete-Design.md should be present.
        // Minimal, conservative set of JSON-RPC endpoints with placeholders.

        // Example: Ping method to verify plugin is responsive.
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
        /** JSON-RPC: ping
         * @param params JSON input with optional "message"
         * @param result JSON output with "reply"
         * @return Core::ERROR_NONE on success
         */

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
        /** JSON-RPC: getpermissions (returns a list of permissions for given appId). */

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
        /** JSON-RPC: invalidatepermissions (invalidates cache for given appId). */

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
        /** JSON-RPC: updatepermissionscache (fetch and update permissions cache for given appId). */

        // Required event: statechanged (typical for service plugins to signal state)
        // PUBLIC_INTERFACE
        void EventStateChanged(const string& state);
        /** Notify all JSON-RPC subscribers that the state changed. */

    private:
        void RegisterAll();
        void UnregisterAll();

        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        PluginHost::IShell* mService;
        OttServicesImplementation* _implementation;
        Exchange::IOttServices* _interface; // COMRPC interface pointer exposed via aggregation
        string _version;

        // Store remote connection id (if using out-of-process implementation); 0 if in-process.
        uint32_t mConnectionId;

        // Remote connection notification sink (registered with IShell)
        Core::Sink<RemoteConnectionNotification> _notification;

        // Non-copyable utility to guard thread-safety if needed in future extensions.
        mutable Core::CriticalSection _adminLock;
    };

} // namespace Plugin
} // namespace WPEFramework
