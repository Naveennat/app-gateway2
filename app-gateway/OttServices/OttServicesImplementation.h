#pragma once

// Internal implementation for the OttServices plugin.
// Contains business logic and state, separated from the Thunder plugin facade.

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IShell.h>
#include <memory>
#include <vector>
#include <string>

#include "Module.h"
#include <interfaces/IOttServices.h>
#include <interfaces/IConfiguration.h>

// Permissions client and cache utils
#include "PermissionsClient.h"

namespace WPEFramework {
namespace Plugin {

    class OttServicesImplementation : public Exchange::IOttServices, public Exchange::IConfiguration {
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

        // IConfiguration
        // PUBLIC_INTERFACE
        uint32_t Configure(PluginHost::IShell* shell) override;

        BEGIN_INTERFACE_MAP(OttServicesImplementation)
            INTERFACE_ENTRY(Exchange::IOttServices)
            INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

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
    };

} // namespace Plugin
} // namespace WPEFramework
