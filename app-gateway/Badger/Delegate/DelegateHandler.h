/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2020 RDK Management
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
**/

#pragma once

#include "DisplaySettingsDelegate.h"
#include "HdcpProfileDelegate.h"
#include "NetworkDelegate.h"
#include "SystemDelegate.h"
#include "AuthServiceDelegate.h"
#include "AdvertisingDelegate.h"
#include "DiscoveryDelegate.h"

namespace WPEFramework {
    class DelegateHandler {
      public:
        DelegateHandler()
            : displaySettingsDelegate(nullptr), hdcpProfileDelegate(nullptr), networkDelegate(nullptr), systemDelegate(nullptr), authServiceDelegate(nullptr), advertisingDelegate(nullptr),
              discoveryDelegate(nullptr) {}

        ~DelegateHandler() {
            displaySettingsDelegate = nullptr;
            hdcpProfileDelegate = nullptr;
            networkDelegate = nullptr;
            systemDelegate = nullptr;
            authServiceDelegate = nullptr;
            advertisingDelegate = nullptr;
            discoveryDelegate = nullptr;
        }

        void setShell(PluginHost::IShell* shell) {
            ASSERT(shell != nullptr);
            LOGDBG("DelegateHandler::setShell");

            if (displaySettingsDelegate == nullptr) {
                displaySettingsDelegate = std::make_shared<DisplaySettingsDelegate>(shell);
            }
            if (hdcpProfileDelegate == nullptr) {
                hdcpProfileDelegate = std::make_shared<HdcpProfileDelegate>(shell);
            }
            if (networkDelegate == nullptr) {
                networkDelegate = std::make_shared<NetworkDelegate>(shell);
            }
            if (systemDelegate == nullptr) {
                systemDelegate = std::make_shared<SystemDelegate>(shell);
            }
            if (authServiceDelegate == nullptr) {
                authServiceDelegate = std::make_shared<AuthServiceDelegate>(shell);
            }
            if (advertisingDelegate == nullptr) {
                advertisingDelegate = std::make_shared<AdvertisingDelegate>(shell);
            }
            if (discoveryDelegate == nullptr) {
                discoveryDelegate = std::make_shared<DiscoveryDelegate>(shell);
            }
        }

        void Cleanup() {
            displaySettingsDelegate.reset();
            hdcpProfileDelegate.reset();
            networkDelegate.reset();
            systemDelegate.reset();
            authServiceDelegate.reset();
            advertisingDelegate.reset();
            discoveryDelegate.reset();
        }

        std::shared_ptr<DisplaySettingsDelegate> getDisplaySettingsDelegate() const { return displaySettingsDelegate; }
        std::shared_ptr<HdcpProfileDelegate> getHdcpProfileDelegate() const { return hdcpProfileDelegate; }
        std::shared_ptr<NetworkDelegate> getNetworkDelegate() const { return networkDelegate; }
        std::shared_ptr<SystemDelegate> getSystemDelegate() const { return systemDelegate; }
        std::shared_ptr<AuthServiceDelegate> getAuthServiceDelegate() const { return authServiceDelegate; }
        std::shared_ptr<AdvertisingDelegate> getAdvertisingDelegate() const { return advertisingDelegate; }
        std::shared_ptr<DiscoveryDelegate> getDiscoveryDelegate() const { return discoveryDelegate; }

      private:
        std::shared_ptr<DisplaySettingsDelegate> displaySettingsDelegate;
        std::shared_ptr<HdcpProfileDelegate> hdcpProfileDelegate;
        std::shared_ptr<NetworkDelegate> networkDelegate;
        std::shared_ptr<SystemDelegate> systemDelegate;
        std::shared_ptr<AuthServiceDelegate> authServiceDelegate;
        std::shared_ptr<AdvertisingDelegate> advertisingDelegate;
        std::shared_ptr<DiscoveryDelegate> discoveryDelegate;
    };
}  // namespace WPEFramework