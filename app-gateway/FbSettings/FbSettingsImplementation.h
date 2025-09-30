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
#include <interfaces/IFbSettings.h>
#include <interfaces/IConfiguration.h>
#include <mutex>
#include <map>
#include "UtilsLogging.h"
#include "ThunderUtils.h"
#include "delegate/SettingsDelegate.h"
namespace WPEFramework {
namespace Plugin {
    class FbSettingsImplementation : public Exchange::IFbSettings, public Exchange::IConfiguration {
    private:

        FbSettingsImplementation(const FbSettingsImplementation&) = delete;
        FbSettingsImplementation& operator=(const FbSettingsImplementation&) = delete;

        class EXTERNAL EventRegistrationJob : public Core::IDispatch 
        {
            protected:
                EventRegistrationJob(FbSettingsImplementation *parent,
                const string event, 
                const bool listen):mParent(*parent),mEvent(event),mListen(listen){

                }
            public:
                EventRegistrationJob() = delete;
                EventRegistrationJob(const EventRegistrationJob &) = delete;
                EventRegistrationJob &operator=(const EventRegistrationJob &) = delete;
                ~EventRegistrationJob()
                {
                }

                static Core::ProxyType<Core::IDispatch> Create(FbSettingsImplementation *parent,
                const string& event, const bool listen)
                {
                    return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<EventRegistrationJob>::Create(parent, event, listen)));
                }
                virtual void Dispatch()
                {
                    mParent.mDelegate->HandleAppEventNotifier(mEvent, mListen);;
                }

            private:
            FbSettingsImplementation &mParent;
            const string mEvent;
            const bool mListen;

        };

    public:
        FbSettingsImplementation();
        ~FbSettingsImplementation();

        BEGIN_INTERFACE_MAP(FbSettingsImplementation)
        INTERFACE_ENTRY(Exchange::IFbSettings)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

        Core::hresult HandleAppEventNotifier(const string event, const bool listen, bool& status /* @out */) override;

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);

    private:
        PluginHost::IShell* mShell;
        std::shared_ptr<SettingsDelegate> mDelegate;
    };
}
}
