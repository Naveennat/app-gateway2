#pragma once

// Internal implementation for the OttServices plugin.
// Contains business logic and state, separated from the Thunder plugin facade.

#include <atomic>
#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IShell.h>
#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include <utility>

#include "Module.h"
#include <OttServices/interfaces/IOttServices.h>

// Permissions client and cache utils
#include "PermissionsClient.h"

namespace WPEFramework {
namespace Plugin {

    class OttServicesImplementation : public Exchange::IOttServices {
    public:
        OttServicesImplementation(const OttServicesImplementation&) = delete;
        OttServicesImplementation& operator=(const OttServicesImplementation&) = delete;

        // PUBLIC_INTERFACE
        OttServicesImplementation();
        // PUBLIC_INTERFACE
        ~OttServicesImplementation() override;

        // PUBLIC_INTERFACE
        string Initialize(PluginHost::IShell* service);
        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service);

        // IUnknown implementation
        // PUBLIC_INTERFACE
        void AddRef() const override;
        // PUBLIC_INTERFACE
        uint32_t Release() const override;
        // PUBLIC_INTERFACE
        void* QueryInterface(const uint32_t id) override;

        // Exchange::IOttServices implementation
        // PUBLIC_INTERFACE
        Core::hresult Ping(const string& message, string& reply) override;

        // PUBLIC_INTERFACE
        Core::hresult GetPermissions(const string& appId, string& permissionsJson) override;

        // Retained vector-returning variant used by JSON-RPC endpoint for local marshalling convenience.
        // PUBLIC_INTERFACE
        Core::hresult GetPermissions(const string& appId, std::vector<string>& permissionsOut);

        // PUBLIC_INTERFACE
        Core::hresult InvalidatePermissions(const string& appId) override;
        // PUBLIC_INTERFACE
        Core::hresult UpdatePermissionsCache(const string& appId, uint32_t& updatedCount) override;

    private:
        struct Config : public Core::JSON::Container {
            Config() : Core::JSON::Container(), PermissionsEndpoint(), UseTls(true) {
                Add(_T("PermissionsEndpoint"), &PermissionsEndpoint);
                Add(_T("UseTls"), &UseTls);
            }
            Core::JSON::String PermissionsEndpoint;
            Core::JSON::Boolean UseTls;
        };

        bool CollectAuthMetadata(std::string& bearerToken,
                                 std::string& deviceId,
                                 std::string& accountId,
                                 std::string& partnerId) const;

    private:
        PluginHost::IShell* _service;
        string _state;

        std::unique_ptr<PermissionsClient> _perms;
        std::string _permsEndpoint;
        bool _permsUseTls;

        // Reference counter for COM-style lifetime management.
        mutable std::atomic<uint32_t> _refCount {1};
    };

} // namespace Plugin
} // namespace WPEFramework
