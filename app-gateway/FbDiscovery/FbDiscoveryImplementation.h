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

#include "Module.h"
#include "UtilsLogging.h"
#include <interfaces/IFbDiscovery.h>
#include <interfaces/IConfiguration.h>
#include <memory>
#include <mutex>
#include <set>

namespace WPEFramework {
namespace Plugin {
    class FbDiscoveryImplementation : public Exchange::IFbDiscovery, public Exchange::IConfiguration {
    private:
        FbDiscoveryImplementation(const FbDiscoveryImplementation&) = delete;
        FbDiscoveryImplementation& operator=(const FbDiscoveryImplementation&) = delete;

    public:
        FbDiscoveryImplementation();
        ~FbDiscoveryImplementation();

        BEGIN_INTERFACE_MAP(FbDiscoveryImplementation)
        INTERFACE_ENTRY(Exchange::IFbDiscovery)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);

        // IFbDiscovery interface
        Core::hresult ClearContentAccess(const Exchange::IFbDiscovery::Context& context) override;
        Core::hresult ContentAccess(const Exchange::IFbDiscovery::Context& context,
                        const string& ids) override;
        Core::hresult SignIn(const Exchange::IFbDiscovery::Context& context,
                     const string& entitlements,
                     bool& success) override;
        Core::hresult SignOut(const Exchange::IFbDiscovery::Context& context,
                      bool& success) override;
        Core::hresult Watched(const Exchange::IFbDiscovery::Context& context,
                      const string& entityId,
                      const double progress,
                      const bool completed,
                      const string& watchedOn,
                      bool& success) override;
        Core::hresult WatchNext(const Exchange::IFbDiscovery::Context& context,
                    const string& title,
                    const string& identifiers,
                    const string& expires,
                    const string& images,
                    bool& success) override;

    private:
        PluginHost::IShell* mShell;

    };
}
}
