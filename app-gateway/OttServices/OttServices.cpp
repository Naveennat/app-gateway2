#include "OttServices.h"
#include "OttServicesImplementation.h"

#include <core/Enumerate.h>
#include <plugins/plugins.h>

namespace WPEFramework {
namespace Plugin {

    namespace {
        // Plugin metadata constants
        static constexpr const TCHAR* kOttServicesPluginName = _T("OttServices");
        static constexpr const TCHAR* kOttServicesCategory = _T("app-gateway");
        static constexpr const TCHAR* kOttServicesVersion = _T("1.0.0"); // Adjust if mandated by spec.

        // JSON-RPC method names
        static constexpr const TCHAR* kMethodPing = _T("ping");

        // JSON-RPC event names
        static constexpr const TCHAR* kEventStateChanged = _T("statechanged");
    }

    // Register the service so Controller can instantiate it
    SERVICE_REGISTRATION(OttServices, 1, 0, 0);

    OttServices::OttServices()
        : PluginHost::JSONRPC()
        , _service(nullptr)
        , _implementation(nullptr)
        , _version(kOttServicesVersion)
        , _refCount(1)
        , _adminLock()
    {
        RegisterAll();
    }

    OttServices::~OttServices() {
        UnregisterAll();
        // _implementation is owned by Initialize/Deinitialize lifecycle.
    }

    void OttServices::RegisterAll() {
        Register<OttServices::PingParams, OttServices::PingResult>(kMethodPing, &OttServices::endpoint_ping, this);
    }

    void OttServices::UnregisterAll() {
        Unregister(kMethodPing);
    }

    const string OttServices::Initialize(PluginHost::IShell* service) {
        ASSERT(service != nullptr);
        _service = service;

        // Instantiate implementation
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        _implementation = new (std::nothrow) OttServicesImplementation();
        if (_implementation == nullptr) {
            return string(_T("OttServices: failed to allocate implementation"));
        }

        // Perform any implementation-level initialization
        const string initError = _implementation->Initialize(_service);
        if (initError.empty() == false) {
            delete _implementation;
            _implementation = nullptr;
            return initError;
        }

        return string(); // Empty string indicates success in Thunder plugins
    }

    void OttServices::Deinitialize(PluginHost::IShell* service) {
        Core::SafeSyncType<Core::CriticalSection> guard(_adminLock);
        if (_implementation != nullptr) {
            _implementation->Deinitialize(service);
            delete _implementation;
            _implementation = nullptr;
        }
        _service = nullptr;
    }

    string OttServices::Information() const {
        // Provide plugin descriptive information
        return string(kOttServicesPluginName) + _T(" plugin v") + _version + _T(" (") + kOttServicesCategory + _T(")");
    }

    // IUnknown implementation

    uint32_t OttServices::AddRef() const {
        return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
    }

    uint32_t OttServices::Release() const {
        const uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
        if (count == 0) {
            delete this;
        }
        return count;
    }

    void* OttServices::QueryInterface(const uint32_t id) {
        void* result = nullptr;

        if (id == PluginHost::IPlugin::ID) {
            result = static_cast<PluginHost::IPlugin*>(this);
        } else if (id == PluginHost::IDispatcher::ID) {
            result = static_cast<PluginHost::IDispatcher*>(this);
        }

        if (result != nullptr) {
            AddRef();
        }
        return result;
    }

    // JSON-RPC method implementation
    uint32_t OttServices::endpoint_ping(const PingParams& params, PingResult& result) {
        // Delegate to implementation
        string reply;
        const uint32_t status = (_implementation != nullptr)
            ? _implementation->Ping((params.Message.IsSet() ? params.Message.Value() : string()), reply)
            : Core::ERROR_UNAVAILABLE;

        if (status == Core::ERROR_NONE) {
            result.Reply = reply;
        }
        return status;
    }

    void OttServices::EventStateChanged(const string& state) {
        // Send a JSON-RPC event to clients
        Core::JSON::String payload;
        payload = state;
        Notify(kEventStateChanged, payload);
    }

} // namespace Plugin
} // namespace WPEFramework
