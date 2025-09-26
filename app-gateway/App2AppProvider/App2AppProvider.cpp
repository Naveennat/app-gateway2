#include "App2AppProvider.h"
#include "App2AppProviderImplementation.h"

namespace WPEFramework {
namespace Plugin {

App2AppProvider::App2AppProvider()
    : _refCount(1)
    , _service(nullptr)
    , _impl()
    , _iface(nullptr) {
    LOGTRACE("App2AppProvider constructed");
}

App2AppProvider::~App2AppProvider() {
    LOGTRACE("App2AppProvider destructed");
}

Exchange::IAppGateway* App2AppProvider::AcquireGateway_(PluginHost::IShell* service) const {
    if (service == nullptr) {
        return nullptr;
    }

    // Try the default callsign "org.rdk.AppGateway" first, then "AppGateway" for fallback.
    Exchange::IAppGateway* gw = service->QueryInterfaceByCallsign<Exchange::IAppGateway>(_T("org.rdk.AppGateway"));
    if (gw == nullptr) {
        gw = service->QueryInterfaceByCallsign<Exchange::IAppGateway>(_T("AppGateway"));
    }
    return gw;
}

// PUBLIC_INTERFACE
const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    LOGTRACE("Initialize enter");
    ASSERT(service != nullptr);
    _service = service;

    // Acquire IAppGateway for outbound responses
    Exchange::IAppGateway* gateway = AcquireGateway_(_service);
    if (gateway == nullptr) {
        LOGERR("IAppGateway not available via IShell::QueryInterfaceByCallsign");
        // Return a textual error; Thunder expects an empty string on success
        return string("App2AppProvider: AppGateway not available");
    }

    // Construct the implementation; it retains the gateway until deinit.
    _impl.reset(new (std::nothrow) App2AppProviderImplementation(gateway));
    if (!_impl) {
        gateway->Release();
        LOGERR("Failed to allocate App2AppProviderImplementation");
        return string("App2AppProvider: memory allocation failed");
    }

    _iface = static_cast<Exchange::IApp2AppProvider*>(_impl.get());
    LOGTRACE("Initialize exit: success");
    return string(); // success
}

// PUBLIC_INTERFACE
void App2AppProvider::Deinitialize(PluginHost::IShell* service) {
    LOGTRACE("Deinitialize enter");
    (void)service;

    if (_impl) {
        // Implementation owns the AppGateway reference and will release it in destructor.
        _impl.reset();
        _iface = nullptr;
    }

    _service = nullptr;
    LOGTRACE("Deinitialize exit");
}

// PUBLIC_INTERFACE
string App2AppProvider::Information() const {
    return string();
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::AddRef() const {
    return _refCount.fetch_add(1, std::memory_order_relaxed) + 1;
}

// PUBLIC_INTERFACE
uint32_t App2AppProvider::Release() const {
    uint32_t count = _refCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
    if (count == 0) {
        delete this;
    }
    return count;
}

// PUBLIC_INTERFACE
void* App2AppProvider::QueryInterface(const uint32_t id) {
    void* result = nullptr;

    if (id == PluginHost::IPlugin::ID) {
        result = static_cast<PluginHost::IPlugin*>(this);
    } else if ((id == Exchange::IApp2AppProvider::ID) && (_iface != nullptr)) {
        _iface->AddRef();
        result = _iface;
    }

    if (result != nullptr) {
        AddRef();
    }
    return result;
}

} // namespace Plugin
} // namespace WPEFramework
