#pragma once

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

#include "DisplaySettingsDelegate.h"
#include "HdcpProfileDelegate.h"
#include "NetworkDelegate.h"
#include "SystemDelegate.h"
#include "MetricsDelegate.h"
#include "UserSettingsDelegate.h"
#include "PrivacyDelegate.h"

using namespace WPEFramework;

class DelegateHandler {
  public:
    DelegateHandler()
        : displaySettingsDelegate(nullptr)
        , hdcpProfileDelegate(nullptr)
        , networkDelegate(nullptr)
        , systemDelegate(nullptr)
        , metricsDelegate(nullptr)
        , userSettingsDelegate(nullptr)
        , privacyDelegate(nullptr) {}

    ~DelegateHandler() {
        displaySettingsDelegate = nullptr;
        hdcpProfileDelegate = nullptr;
        networkDelegate = nullptr;
        systemDelegate = nullptr;
        metricsDelegate = nullptr;
        userSettingsDelegate = nullptr;
        privacyDelegate = nullptr;
    }

    // PUBLIC_INTERFACE
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
        if (metricsDelegate == nullptr) {
            metricsDelegate = std::make_shared<MetricsDelegate>(shell);
        }
        if (userSettingsDelegate == nullptr) {
            userSettingsDelegate = std::make_shared<UserSettingsDelegate>(shell);
        }
        if (privacyDelegate == nullptr) {
            privacyDelegate = std::make_shared<PrivacyDelegate>(shell);
        }
    }

    void Cleanup() { displaySettingsDelegate.reset(); }

    std::shared_ptr<DisplaySettingsDelegate> getDisplaySettingsDelegate() const { return displaySettingsDelegate; }
    std::shared_ptr<HdcpProfileDelegate> getHdcpProfileDelegate() const { return hdcpProfileDelegate; }
    std::shared_ptr<NetworkDelegate> getNetworkDelegate() const { return networkDelegate; }
    std::shared_ptr<SystemDelegate> getSystemDelegate() const { return systemDelegate; }

    // PUBLIC_INTERFACE
    std::shared_ptr<MetricsDelegate> getMetricsDelegate() const { return metricsDelegate; }

    // PUBLIC_INTERFACE
    std::shared_ptr<UserSettingsDelegate> getUserSettingsDelegate() const { return userSettingsDelegate; }

    // PUBLIC_INTERFACE
    std::shared_ptr<PrivacyDelegate> getPrivacyDelegate() const { return privacyDelegate; }

  private:
    std::shared_ptr<DisplaySettingsDelegate> displaySettingsDelegate;
    std::shared_ptr<HdcpProfileDelegate> hdcpProfileDelegate;
    std::shared_ptr<NetworkDelegate> networkDelegate;
    std::shared_ptr<SystemDelegate> systemDelegate;
    std::shared_ptr<MetricsDelegate> metricsDelegate;
    std::shared_ptr<UserSettingsDelegate> userSettingsDelegate;
    std::shared_ptr<PrivacyDelegate> privacyDelegate;
};
