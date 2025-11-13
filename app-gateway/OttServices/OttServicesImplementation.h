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
#include <mutex>

#include "Module.h"
#include <interfaces/IOttServices.h>
#include <interfaces/IConfiguration.h>

// Permissions client and cache utils
#include "PermissionsClient.h"

namespace WPEFramework {
namespace Plugin {

    class TokenClient; // forward declaration

    class OttServicesImplementation : public Exchange::IOttServices , public Exchange::IConfiguration{
    public:
        OttServicesImplementation(const OttServicesImplementation&) = delete;
        OttServicesImplementation& operator=(const OttServicesImplementation&) = delete;

        
        // PUBLIC_INTERFACE
        OttServicesImplementation();
        // PUBLIC_INTERFACE
        ~OttServicesImplementation() override;

        
        BEGIN_INTERFACE_MAP(OttServicesImplementation)
        INTERFACE_ENTRY(Exchange::IOttServices)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

        // PUBLIC_INTERFACE
        string Initialize(PluginHost::IShell* service);
        // PUBLIC_INTERFACE
        void Deinitialize(PluginHost::IShell* service);

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);
    
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

        // ---- Token retrieval placeholders (stubs for future implementation) ----
        // PUBLIC_INTERFACE
        Core::hresult GetDistributorToken(const string& appId,
                                          const string& xact,
                                          const string& sat,
                                          string& tokenJson) override;

        // PUBLIC_INTERFACE
        Core::hresult GetAuthToken(const string& appId,
                                   const string& sat,
                                   string& tokenJson) override;

    private:
        struct Config : public Core::JSON::Container {
            Config() : Core::JSON::Container(), PermissionsEndpoint(), UseTls(true), TokenEndpoint(), UseTlsToken(true) {
                Add(_T("PermissionsEndpoint"), &PermissionsEndpoint);
                Add(_T("UseTls"), &UseTls);

                // Optional ott_token service endpoint; falls back to PermissionsEndpoint if not provided.
                Add(_T("TokenEndpoint"), &TokenEndpoint);
                // Optional TLS flag for token service; falls back to UseTls if not provided.
                Add(_T("UseTlsToken"), &UseTlsToken);
            }
            Core::JSON::String PermissionsEndpoint;
            Core::JSON::Boolean UseTls;

            Core::JSON::String TokenEndpoint;
            Core::JSON::Boolean UseTlsToken;
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

        // Token client and configuration
        std::unique_ptr<TokenClient> _token;
        std::string _tokenEndpoint;
        bool _tokenUseTls;

        // Token path: guard token cache/client access.
        std::mutex _tokenMutex;

        // Reference counter for COM-style lifetime management.
        mutable std::atomic<uint32_t> _refCount {1};
    };

} // namespace Plugin
} // namespace WPEFramework
