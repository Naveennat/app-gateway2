#pragma once

// Internal implementation for the OttServices plugin.
// Contains business logic and state, separated from the Thunder plugin facade.

#include <core/core.h>
#include <core/JSON.h>
#include <plugins/IShell.h>
#include <memory>
#include <vector>
#include <string>

// Permissions client and cache utils
#include "PermissionsClient.h"

namespace WPEFramework {
namespace Plugin {

    class OttServicesImplementation {
    public:
        OttServicesImplementation(const OttServicesImplementation&) = delete;
        OttServicesImplementation& operator=(const OttServicesImplementation&) = delete;

        // PUBLIC_INTERFACE
        OttServicesImplementation();
        // PUBLIC_INTERFACE
        ~OttServicesImplementation();

        // PUBLIC_INTERFACE
        string Initialize(PluginHost::IShell* service);
        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service);

        // PUBLIC_INTERFACE
        uint32_t Ping(const string& message, string& reply);

        // PUBLIC_INTERFACE
        uint32_t GetPermissions(const string& appId, std::vector<string>& permissionsOut);
        // PUBLIC_INTERFACE
        uint32_t InvalidatePermissions(const string& appId);
        // PUBLIC_INTERFACE
        uint32_t UpdatePermissionsCache(const string& appId, uint32_t& updatedCount);

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
