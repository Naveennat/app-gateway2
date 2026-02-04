/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
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
#include <interfaces/IOttServices.h>
#include <interfaces/IConfiguration.h>
#include "OttPermissionCache.h"

// Permissions client and cache utils
#include "PermissionsClient.h"
#include "TokenCache.h"
#include "TokenClient.h"
#include "UtilsLogging.h"


namespace WPEFramework {
namespace Plugin {

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
        Core::hresult GetAppPermissions(const string& appId, bool forceNew, Exchange::IOttServices::IStringIterator*& permissions) const override;

        // PUBLIC_INTERFACE
        Core::hresult GetAppCIMAToken(const string& appId, string& token) const override;

        // PUBLIC_INTERFACE
        Core::hresult GetAppThorToken(const string& appId, 
                                           const string& contentProvider, 
                                           const string& deviceSessionId,
                                           const string& appSessionId, 
                                           string& token )  const override;                              

    private:
        struct Config : public Core::JSON::Container {
            Config() : Core::JSON::Container(), PermissionsEndpoint(), UseTls(true) {
                Add(_T("PermissionsEndpoint"), &PermissionsEndpoint);
                Add(_T("UseTls"), &UseTls);
                Add(_T("TokenEndpoint"), &TokenEndpoint);
                Add(_T("UseTlsToken"), &UseTlsToken);
            }
            Core::JSON::String PermissionsEndpoint;
            Core::JSON::Boolean UseTls;
            Core::JSON::String TokenEndpoint;
            Core::JSON::Boolean UseTlsToken;
        };

        void SetPermissionsClient() const;
        void SetTokenClient() const;

        bool CollectAuthMetadata(std::string& bearerToken,
                                 std::string& deviceId,
                                 std::string& accountId,
                                 std::string& partnerId) const;
        // Helpers to fetch SAT and xACT via AuthService
        bool FetchSat(std::string& sat /* @out */, uint64_t& expiryEpoch /* @out */) const;
        bool FetchXact(const std::string& appId /* @in */, std::string& xact /* @out */, uint64_t& expiryEpoch /* @out */) const;
        bool FetchXsct(std::string& xSCT, uint64_t& expiryEpoch) const;
        Core::hresult GetPermissionsFromServer(const string& appId, std::vector<std::string>& permissions) const;
        Core::hresult UpdatePermissions(const string& appId, std::vector<std::string>& permissions) const;
        // Utility: current epoch seconds and JSON parsing for expires_in
        static uint64_t NowEpochSec();
        // Removed: ExtractExpiresInSeconds no longer needed (token methods return raw strings).

        string GetAccountId() const;
        string GetPartnerId() const;
        string GetDeviceId() const;
    
    private:
            class EXTERNAL UpdatePermissionCacheJob : public Core::IDispatch
            {
                protected:
                    UpdatePermissionCacheJob(
                                            const string& appId,
                                            const std::vector<std::string>& permissions)
                        :  mAppId(appId), mPermissions(permissions)
                    {
                    }
                
                public:
                    UpdatePermissionCacheJob() = delete;
                    UpdatePermissionCacheJob(const UpdatePermissionCacheJob &) = delete;
                    UpdatePermissionCacheJob &operator=(const UpdatePermissionCacheJob &) = delete;
                    ~UpdatePermissionCacheJob()
                    {
                    }
                
                public:
                    static Core::ProxyType<Core::IDispatch> Create(
                        const string& appId,
                        const std::vector<std::string>& permissions)
                    {
                        return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<UpdatePermissionCacheJob>::Create(appId, permissions)));
                    }
                    virtual void Dispatch()
                    {
                        OttPermissionCache::Instance().UpdateCache(mAppId, mPermissions);
                        uint32_t updatedCount = static_cast<uint32_t>(mPermissions.size());
                        LOGINFO("UpdatePermissionsCache updated (appId='%s', count=%u)", mAppId.c_str(), updatedCount);
                    }
            
                private:
                    const string mAppId;
                    const std::vector<std::string> mPermissions;

            };

    private:
        PluginHost::IShell* _service;
        string _state;

        mutable std::shared_ptr<PermissionsClient> _perms;       
        std::string _permsEndpoint;
        bool _permsUseTls;

        // Token client and configuration
        mutable std::shared_ptr<TokenClient> _token;
        std::string _tokenEndpoint;
        bool _tokenUseTls;

        // Token path: guard token cache/client access.
        mutable std::mutex _tokenMutex;
        mutable TokenCache _tokenCache;

        // Reference counter for COM-style lifetime management.
        mutable std::atomic<uint32_t> _refCount {1};
        mutable bool _permissionsClientInitialized {false};
        mutable bool _tokenClientInitialized {false};
    };

} // namespace Plugin
} // namespace WPEFramework
