 /*
  * SPDX-License-Identifier: Apache-2.0
  *
  * Thin JSON-RPC adapter for IOttServices, modeled after FbSettings.cpp.
  * Delegates to an out-of-process implementation named "OttServicesImplementation".
  */

#include "OttServices.h"
#include "Version.h"

#include <interfaces/IConfiguration.h>
#include <interfaces/json/JsonData_OttServices.h>
#include <interfaces/json/JOttServices.h>

#define API_VERSION_NUMBER_MAJOR    OTTSERVICES_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR    OTTSERVICES_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH    OTTSERVICES_PATCH_VERSION

namespace WPEFramework {

namespace {
    static Plugin::Metadata<Plugin::OttServices> metadata(
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

    SERVICE_REGISTRATION(OttServices, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    OttServices::OttServices()
        : _service(nullptr)
        , _ottServices(nullptr)
        , _connectionId(0)
    {
        SYSLOG(Logging::Startup, (_T("OttServices Constructor")));
    }

    OttServices::~OttServices()
    {
        SYSLOG(Logging::Shutdown, (string(_T("OttServices Destructor"))));
    }

    /* virtual */ const string OttServices::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(_ottServices == nullptr);

        SYSLOG(Logging::Startup, (_T("OttServices::Initialize: PID=%u"), getpid()));

        _service = service;
        _service->AddRef();

        // Acquire the remote IOttServices implementation
        _ottServices = service->Root<Exchange::IOttServices>(_connectionId, 2000, _T("OttServicesImplementation"));

        if (_ottServices != nullptr) {
            // If the remote supports IConfiguration, let it configure with our shell.
            auto configConnection = _ottServices->QueryInterface<Exchange::IConfiguration>();
            if (configConnection != nullptr) {
                configConnection->Configure(service);
                configConnection->Release();
            }

            // Register JSON-RPC methods/events for IOttServices
            Exchange::JOttServices::Register(*this, _ottServices);
        } else {
            SYSLOG(Logging::Startup, (_T("OttServices::Initialize: Failed to initialise OttServices plugin")));
        }

        // On success return empty, to indicate there is no error text.
        return ((_ottServices != nullptr))
            ? EMPTY_STRING
            : _T("Could not retrieve the OttServices interface.");
    }

    /* virtual */ void OttServices::Deinitialize(PluginHost::IShell* service)
    {
        SYSLOG(Logging::Shutdown, (string(_T("OttServices::Deinitialize"))));
        ASSERT(service == _service);

        if (_ottServices != nullptr) {
            Exchange::JOttServices::Unregister(*this);

            RPC::IRemoteConnection* connection(service->RemoteConnection(_connectionId));
            VARIABLE_IS_NOT_USED uint32_t result = _ottServices->Release();
            _ottServices = nullptr;

            // It should have been the last reference we are releasing, so it should end up in a
            // DESTRUCTION_SUCCEEDED, if not we are leaking...
            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            // If this was running in a (container) process...
            if (connection != nullptr) {
                // Trigger a cleanup sequence for out-of-process code.
                connection->Terminate();
                connection->Release();
            }
        }

        _connectionId = 0;
        _service->Release();
        _service = nullptr;

        SYSLOG(Logging::Shutdown, (string(_T("OttServices de-initialised"))));
    }

    void OttServices::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == _connectionId) {

            ASSERT(_service != nullptr);

            Core::IWorkerPool::Instance().Submit(
                PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
