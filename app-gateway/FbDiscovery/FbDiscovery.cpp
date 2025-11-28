/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
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
 */

#include "FbDiscovery.h"
#include "UtilsLogging.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/json/JsonData_FbDiscovery.h>
#include <interfaces/json/JFbDiscovery.h>


#define API_VERSION_NUMBER_MAJOR    FBDISCOVERY_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR    FBDISCOVERY_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH    FBDISCOVERY_PATCH_VERSION

namespace WPEFramework {

namespace {
    static Plugin::Metadata<Plugin::FbDiscovery> metadata(
        // Version (Major, Minor, Patch)
        API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
        // Preconditions
        {},
        // Terminations
        {},
        // Controls
        {}
    );
}

namespace Plugin {
    SERVICE_REGISTRATION(FbDiscovery, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    FbDiscovery::FbDiscovery(): mService(nullptr), mFbDiscovery(nullptr), mConnectionId(0), mNotification(this)
    {
        LOGINFO("FbDiscovery::FbDiscovery Constructor");
    }

    FbDiscovery::~FbDiscovery()
    {
        LOGINFO("FbDiscovery::~FbDiscovery destructor");
    }

    /* virtual */ const string FbDiscovery::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(mFbDiscovery == nullptr);

        LOGINFO("FbDiscovery::Initialize: PID=%u", getpid());

        mService = service;
        mService->AddRef();
        mService->Register(&mNotification);

        mFbDiscovery = service->Root<Exchange::IFbDiscovery>(mConnectionId, 2000, _T("FbDiscoveryImplementation"));

        if (mFbDiscovery != nullptr) {

            auto configConnection = mFbDiscovery->QueryInterface<Exchange::IConfiguration>();
            if (configConnection != nullptr) {
                configConnection->Configure(service);
                configConnection->Release();
            }

            //Invoking Plugin API register to wpeframework
            Exchange::JFbDiscovery::Register(*this, mFbDiscovery);
        }
        else
        {
            LOGINFO("FbDiscovery::Initialize: Failed to initialise FbDiscovery plugin");
        }
        // On success return empty, to indicate there is no error text.
        return ((mFbDiscovery != nullptr))
            ? EMPTY_STRING
            : _T("Could not retrieve the FbDiscovery interface.");
    }

    /* virtual */ void FbDiscovery::Deinitialize(PluginHost::IShell* service)
    {
        LOGINFO("FbDiscovery de-initialising");
        ASSERT(service == mService);

        mService->Unregister(&mNotification);

        if (mFbDiscovery != nullptr) {

            Exchange::JFbDiscovery::Unregister(*this);

            RPC::IRemoteConnection *connection(service->RemoteConnection(mConnectionId));
            VARIABLE_IS_NOT_USED uint32_t result = mFbDiscovery->Release();
            mFbDiscovery = nullptr;

            // It should have been the last reference we are releasing,
            // so it should end up in a DESCRUCTION_SUCCEEDED, if not we
            // are leaking...
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            // If this was running in a (container) process...
            if (connection != nullptr)
            {
                // Lets trigger a cleanup sequence for
                // out-of-process code. Which will guard
                // that unwilling processes, get shot if
                // not stopped friendly :~)
                connection->Terminate();
                connection->Release();
            }
        }

        mConnectionId = 0;
        mService->Release();
        mService = nullptr;
        LOGINFO("FbDiscovery de-initialised");
    }

    void FbDiscovery::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == mConnectionId) {

            ASSERT(mService != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework