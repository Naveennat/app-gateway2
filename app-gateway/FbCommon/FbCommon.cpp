#include "FbCommon.h"
#include <interfaces/IConfiguration.h>
#include <unistd.h>

#define API_VERSION_NUMBER_MAJOR    1
#define API_VERSION_NUMBER_MINOR    0
#define API_VERSION_NUMBER_PATCH    0

namespace WPEFramework {

namespace {
    static Plugin::Metadata<Plugin::FbCommon> metadata(
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
    SERVICE_REGISTRATION(FbCommon, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

    FbCommon::FbCommon()
        : mService(nullptr)
        , mRequestHandler(nullptr)
        , mConnectionId(0)
    {
        SYSLOG(Logging::Startup, (_T("FbCommon Constructor")));
    }

    FbCommon::~FbCommon()
    {
        SYSLOG(Logging::Shutdown, (string(_T("FbCommon Destructor"))));
    }

    const string FbCommon::Initialize(PluginHost::IShell* service)
    {
        ASSERT(service != nullptr);
        ASSERT(mRequestHandler == nullptr);

        SYSLOG(Logging::Startup, (_T("FbCommon::Initialize: PID=%u"), getpid()));

        mService = service;
        mService->AddRef();

        // Acquire the implementation exposing IAppGatewayRequestHandler over COM-RPC
        mRequestHandler = service->Root<Exchange::IAppGatewayRequestHandler>(mConnectionId, 2000, _T("FbCommonImplementation"));

        if (mRequestHandler != nullptr) {
            // Optionally configure the implementation, if it also exposes IConfiguration
            auto configConnection = mRequestHandler->QueryInterface<Exchange::IConfiguration>();
            if (configConnection != nullptr) {
                configConnection->Configure(service);
                configConnection->Release();
            }
        } else {
            SYSLOG(Logging::Startup, (_T("FbCommon::Initialize: Failed to initialize FbCommon implementation")));
        }

        return ((mRequestHandler != nullptr))
            ? EMPTY_STRING
            : _T("Could not retrieve the FbCommon interface (IAppGatewayRequestHandler).");
    }

    void FbCommon::Deinitialize(PluginHost::IShell* service)
    {
        SYSLOG(Logging::Shutdown, (string(_T("FbCommon::Deinitialize"))));
        ASSERT(service == mService);

        if (mRequestHandler != nullptr) {
            RPC::IRemoteConnection* connection(service->RemoteConnection(mConnectionId));
            VARIABLE_IS_NOT_USED uint32_t result = mRequestHandler->Release();
            mRequestHandler = nullptr;

            ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

            if (connection != nullptr) {
                connection->Terminate();
                connection->Release();
            }
        }

        mConnectionId = 0;
        mService->Release();
        mService = nullptr;
        SYSLOG(Logging::Shutdown, (string(_T("FbCommon de-initialized"))));
    }

    void FbCommon::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == mConnectionId) {
            ASSERT(mService != nullptr);
            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

} // namespace Plugin
} // namespace WPEFramework
