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

#include "FbPrivacy.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/json/JsonData_FbPrivacy.h>
#include <interfaces/json/JFbPrivacy.h>


#define API_VERSION_NUMBER_MAJOR    FBPRIVACY_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR    FBPRIVACY_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH    FBPRIVACY_PATCH_VERSION

namespace WPEFramework {

namespace {
    static Plugin::Metadata<Plugin::FbPrivacy> metadata(
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
    SERVICE_REGISTRATION(FbPrivacy, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    FbPrivacy::FbPrivacy(): mService(nullptr), mPluginHandler(nullptr), mFbPrivacy(nullptr), mAppNotificationHandler(nullptr), mConnectionId(0), mNotification(this)
    {
        SYSLOG(Logging::Startup, (_T("FbPrivacy Constructor")));
    }

    FbPrivacy::~FbPrivacy()
    {
        SYSLOG(Logging::Shutdown, (string(_T("FbPrivacy Destructor"))));
    }

    /* virtual */ const string FbPrivacy::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(mFbPrivacy == nullptr);

        SYSLOG(Logging::Startup, (_T("FbPrivacy::Initialize: PID=%u"), getpid()));

        mService = service;
        mService->AddRef();
        mService->Register(&mNotification);

        mPluginHandler = service->Root<WPEFramework::Core::IUnknown>(mConnectionId, 2000, _T("FbPrivacyImplementation"));

        if (mPluginHandler != nullptr) {

            mFbPrivacy = mPluginHandler->QueryInterface<Exchange::IFbPrivacy>();
            if (mFbPrivacy == nullptr) {
                SYSLOG(Logging::Startup, (_T("FbPrivacy::Initialize: Failed to retrieve the FbPrivacy interface")));
                return _T("Failed to retrieve the FbPrivacy interface.");
            }

            mAppNotificationHandler = mPluginHandler->QueryInterface<Exchange::IAppNotificationHandler>();
            if (mAppNotificationHandler == nullptr) {
                SYSLOG(Logging::Startup, (_T("FbPrivacy::Initialize: Failed to retrieve the IAppNotificationHandler interface")));
                return _T("Failed to retrieve the IAppNotificationHandler interface.");
            }

            // Register for notifications
            mFbPrivacy->Register(&mNotification);

            auto configConnection = mFbPrivacy->QueryInterface<Exchange::IConfiguration>();
            if (configConnection != nullptr) {
                configConnection->Configure(service);
                configConnection->Release();
            }

            //Invoking Plugin API register to wpeframework
            Exchange::JFbPrivacy::Register(*this, mFbPrivacy);
        }
        else
        {
            SYSLOG(Logging::Startup, (_T("FbPrivacy::Initialize: Failed to initialise FbPrivacy plugin")));
        }
        // On success return empty, to indicate there is no error text.
        return ((mPluginHandler != nullptr))
            ? EMPTY_STRING
            : _T("Could not retrieve the FbPrivacy interface.");
    }

    /* virtual */ void FbPrivacy::Deinitialize(PluginHost::IShell* service)
    {
        SYSLOG(Logging::Shutdown, (string(_T("FbPrivacy::Deinitialize"))));
        ASSERT(service == mService);

        mService->Unregister(&mNotification);

        if (mFbPrivacy != nullptr) {

            mFbPrivacy->Unregister(&mNotification);
            Exchange::JFbPrivacy::Unregister(*this);

            VARIABLE_IS_NOT_USED uint32_t result = mFbPrivacy->Release();
            mFbPrivacy = nullptr;

        }

        if (mAppNotificationHandler != nullptr) {
            VARIABLE_IS_NOT_USED uint32_t result = mAppNotificationHandler->Release();
            mAppNotificationHandler = nullptr;
        }

        if (mPluginHandler != nullptr) {
            RPC::IRemoteConnection *connection(service->RemoteConnection(mConnectionId));
            VARIABLE_IS_NOT_USED uint32_t result = mPluginHandler->Release();
            mPluginHandler = nullptr;

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
        SYSLOG(Logging::Shutdown, (string(_T("FbPrivacy de-initialised"))));
    }

    void FbPrivacy::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == mConnectionId) {

            ASSERT(mService != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework