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

#include "AppGateway.h"
#include <interfaces/IConfiguration.h>
#include <interfaces/json/JsonData_AppGatewayResolver.h>
#include <interfaces/json/JAppGatewayResolver.h>
#include "UtilsLogging.h"


#define API_VERSION_NUMBER_MAJOR    APPGATEWAY_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR    APPGATEWAY_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH    APPGATEWAY_PATCH_VERSION


namespace WPEFramework {

namespace {
    static Plugin::Metadata<Plugin::AppGateway> metadata(
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
    SERVICE_REGISTRATION(AppGateway, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    AppGateway::AppGateway()
            : PluginHost::JSONRPC(), mService(nullptr), mAppGateway(nullptr), mResponder(nullptr), mConnectionId(0)
        {

        LOGINFO("AppGateway Constructor");
    }

    AppGateway::~AppGateway()
    {
    }

    /* virtual */ const string AppGateway::Initialize(PluginHost::IShell* service)
    {
        // In production Thunder will never call Initialize with a nullptr.
        // In our isolated L0 harness (in-proc), a nullptr can still occur if the
        // test accidentally passes an invalid pointer; guard to avoid segfaults.
        if (service == nullptr) {
            LOGERR("AppGateway::Initialize called with nullptr service");
            return _T("Invalid service (nullptr).");
        }

        // The upstream plugin expects Initialize to be called once per instance.
        // Our L0 tests may re-initialize on the same instance to validate lifecycle behavior.
        // If we already have a service reference, release it first to avoid leaking and
        // to prevent dereferencing stale pointers.
        if (mService != nullptr) {
            LOGWARN("AppGateway::Initialize called while a service is already set; releasing previous service reference");
            mService->Release();
            mService = nullptr;
        }

        // If a previous Initialize partially succeeded, we may still have aggregates.
        // Clean them up to keep Initialize idempotent in test harness environments.
        if (mResponder != nullptr) {
            LOGWARN("AppGateway::Initialize called while responder is still set; releasing previous responder");
            mResponder->Release();
            mResponder = nullptr;
        }
        if (mAppGateway != nullptr) {
            LOGWARN("AppGateway::Initialize called while resolver is still set; unregistering and releasing previous resolver");
            Exchange::JAppGatewayResolver::Unregister(*this);
            mAppGateway->Release();
            mAppGateway = nullptr;
        }

        LOGINFO("AppGateway::Initialize: PID=%u", getpid());

        mService = service;
        // Defensive: even after assignment, ensure non-null before AddRef.
        if (mService == nullptr) {
            LOGERR("AppGateway::Initialize internal error: service became nullptr");
            return _T("Invalid service (nullptr).");
        }
        mService->AddRef();

        mAppGateway = service->Root<Exchange::IAppGatewayResolver>(mConnectionId, 2000, _T("AppGatewayImplementation"));
       
        if (mAppGateway != nullptr) {
            auto configConnection = mAppGateway->QueryInterface<Exchange::IConfiguration>();
            if (configConnection != nullptr) {
                configConnection->Configure(service);
                configConnection->Release();
            }

            //Invoking Plugin API register to wpeframework
            Exchange::JAppGatewayResolver::Register(*this, mAppGateway);
        }
        else
        {
            LOGERR("Failed to initialise AppGatewayResolver plugin!");
        }

        mResponder = service->Root<Exchange::IAppGatewayResponder>(mConnectionId, 2000, _T("AppGatewayResponderImplementation"));
        if (mResponder != nullptr) {
            auto configConnectionResponder = mResponder->QueryInterface<Exchange::IConfiguration>();
            if (configConnectionResponder != nullptr) {
                configConnectionResponder->Configure(service);
                configConnectionResponder->Release();
            }
        }
        else
        {
            LOGERR("Failed to initialise AppGatewayResponder plugin!");
        }
   
            
        // On success return empty, to indicate there is no error text.
        return ((mAppGateway != nullptr) && (mResponder != nullptr))
            ? EMPTY_STRING
            : _T("Could not retrieve the AppGateway interface.");
    }

    /* virtual */ void AppGateway::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == mService);

        RPC::IRemoteConnection *connection = nullptr;
        VARIABLE_IS_NOT_USED uint32_t result = Core::ERROR_NONE;

        if ((mAppGateway != nullptr) || (mResponder != nullptr)) {
            connection = service->RemoteConnection(mConnectionId);
        }

        if (mResponder != nullptr) {
            result = mResponder->Release();
            mResponder = nullptr;

            // It should have been the last reference we are releasing,
            // so it should end up in a DESTRUCTION_SUCCEEDED, if not we
            // are leaking...
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

        if (mAppGateway != nullptr) {
            Exchange::JAppGatewayResolver::Unregister(*this);
            result = mAppGateway->Release();
            mAppGateway = nullptr;

            // It should have been the last reference we are releasing,
            // so it should end up in a DESTRUCTION_SUCCEEDED, if not we
            // are leaking...
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);
        }

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

        mConnectionId = 0;
        mService->Release();
        mService = nullptr;
    }

    void AppGateway::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == mConnectionId) {

            ASSERT(mService != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
