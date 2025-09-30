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
#include "FbSettingsImplementation.h"
#include "UtilsLogging.h"
#include "delegate/SettingsDelegate.h"

namespace WPEFramework
{
    namespace Plugin
    {

        SERVICE_REGISTRATION(FbSettingsImplementation, 1, 0);

        FbSettingsImplementation::FbSettingsImplementation() : 
        mShell(nullptr)   
        {
            mDelegate = std::make_shared<SettingsDelegate>();
        }

        FbSettingsImplementation::~FbSettingsImplementation()
        {
            // Cleanup resources if needed
            if (mShell != nullptr)
            {
                mShell->Release();
                mShell = nullptr;
            }
        }

        Core::hresult FbSettingsImplementation::HandleAppEventNotifier(const string event /* @in */,
                                    const bool listen /* @in */,
                                    bool &status /* @out */) {
            LOGINFO("HandleFireboltNotifier [event=%s listen=%s]",
                    event.c_str(), listen ? "true" : "false");
            status = true;
            Core::IWorkerPool::Instance().Submit(EventRegistrationJob::Create(this, event, listen));
            return Core::ERROR_NONE;
        }

        uint32_t FbSettingsImplementation::Configure(PluginHost::IShell *shell)
        {
            LOGINFO("Configuring FbSettings");
            uint32_t result = Core::ERROR_NONE;
            ASSERT(shell != nullptr);
            mShell = shell;
            mShell->AddRef();
            mDelegate->setShell(mShell);
            return result;
        }
    }
}