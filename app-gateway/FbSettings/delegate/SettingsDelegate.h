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
#ifndef __SETTINGSDELEGATE_H__
#define __SETTINGSDELEGATE_H__

#include "StringUtils.h"
#include "BaseEventDelegate.h"
#include "TTSDelegate.h"
#include "UserSettingsDelegate.h"
#include "SystemDelegate.h"
#include "UtilsLogging.h"
#include <interfaces/IAppNotifications.h>
#include <memory>
#include <vector>
#include <string>

#define APP_NOTIFICATIONS_CALLSIGN "org.rdk.AppNotifications"
using namespace WPEFramework;
using WPEFramework::Plugin::SystemDelegate;

class SettingsDelegate {
public:
    SettingsDelegate() = default;

    ~SettingsDelegate() {
        tts = nullptr;
        userSettings = nullptr;
        system = nullptr;
        if (mAppNotifications != nullptr) {
            mAppNotifications->Release();
            mAppNotifications = nullptr;
        }
    }

    void HandleAppEventNotifier(const string event, const bool listen) {
        LOGDBG("Passing on HandleAppEventNotifier");
        bool registrationError = false;
        if (tts == nullptr || userSettings == nullptr || system == nullptr) {
            LOGERR("Services not available");
            return;
        }

        std::vector<std::shared_ptr<BaseEventDelegate>> delegates = { tts, userSettings, system };
        bool handled = false;

        for (const auto& delegate : delegates) {
            if (delegate == nullptr) {
                continue;
            }
            if (delegate->HandleEvent(event, listen, registrationError)) {
                handled = true;
                break;
            }
        }

        if (!handled) {
            LOGERR("No Matching registrations");
        }

        if (registrationError) {
            LOGERR("Error in registering/unregistering for event %s", event.c_str());
        }
    }

    void setShell(PluginHost::IShell* shell) {
        ASSERT(shell != nullptr);
        LOGDBG("SettingsDelegate::setShell");

        mAppNotifications = shell->QueryInterfaceByCallsign<Exchange::IAppNotifications>(APP_NOTIFICATIONS_CALLSIGN);

        if (mAppNotifications == nullptr) {
            LOGERR("No App Notifications plugin available");
            return;
        }

        mAppNotifications->AddRef();

        if (tts == nullptr) {
            tts = std::make_shared<TTSDelegate>(shell, mAppNotifications);
        }

        if (userSettings == nullptr) {
            userSettings = std::make_shared<UserSettingsDelegate>(shell, mAppNotifications);
        }

        if (system == nullptr) {
            system = std::make_shared<SystemDelegate>(shell, mAppNotifications);
        }
    }

    // PUBLIC_INTERFACE
    std::shared_ptr<SystemDelegate> getSystemDelegate() const {
        /** Expose the SystemDelegate instance for system-level alias methods. */
        return system;
    }

private:
    std::shared_ptr<TTSDelegate> tts;
    std::shared_ptr<UserSettingsDelegate> userSettings;
    std::shared_ptr<SystemDelegate> system;
    Exchange::IAppNotifications* mAppNotifications { nullptr };
};

#endif
