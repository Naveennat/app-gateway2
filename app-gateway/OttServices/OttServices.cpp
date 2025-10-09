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

#include <unistd.h> // getpid

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
        _degraded = false;

        // Attempt to acquire the out-of-process IOttServices implementation
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

            SYSLOG(Logging::Startup, (_T("OttServices::Initialize: IOttServices bound via Root() [connId=%u]"), _connectionId));
        } else {
            _degraded = true;

            // Provide actionable diagnostics to operators - keep plugin active in degraded mode.
            SYSLOG(Logging::Startup, (_T("OttServices::Initialize: IOttServices retrieval failed; starting in DEGRADATED mode"))));

            SYSLOG(Logging::Startup, (_T("OttServices::Initialize: Action required:")));
            SYSLOG(Logging::Startup, (_T("  1) Ensure a plugin entry exists for class 'OttServicesImplementation' (callsign can vary)")));
            SYSLOG(Logging::Startup, (_T("     Example (plugins/OttServicesImplementation.json):")));
            SYSLOG(Logging::Startup, (_T("       { \"callsign\": \"OttServicesImplementation\", \"locator\": \"libOttServicesImplementation.so\",")));
            SYSLOG(Logging::Startup, (_T("         \"classname\": \"OttServicesImplementation\", \"autostart\": true }")));
            SYSLOG(Logging::Startup, (_T("  2) Ensure COM-RPC Proxy/Stub for Exchange::IOttServices is built and discoverable")));
            SYSLOG(Logging::Startup, (_T("     - ID must match ID_OTT_SERVICES (0x0000F812) on both sides")));
            SYSLOG(Logging::Startup, (_T("     - The Proxy/Stub library must be under the configured proxystub path")));
            SYSLOG(Logging::Startup, (_T("  3) If using a different class/callsign, update the Root() className or plugin config accordingly")));
            SYSLOG(Logging::Startup, (_T("  4) Optionally activate the implementation via Controller.LifeTime.Activate")));
            SYSLOG(Logging::Startup, (_T("OttServices will remain active but without a bound IOttServices interface until the above is corrected.")));
        }

        // Always return EMPTY_STRING to avoid failing activation; degraded mode is signaled via logs.
        return EMPTY_STRING;
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
        _degraded = false;

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
