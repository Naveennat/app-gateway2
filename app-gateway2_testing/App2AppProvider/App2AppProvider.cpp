#include "App2AppProvider.h"
#include "App2AppProviderImplementation.h"

#include <plugins/plugins.h>

namespace WPEFramework {
namespace Plugin {

SERVICE_REGISTRATION(App2AppProvider, 1, 0, 0);

App2AppProvider::App2AppProvider()
    : _refCount(1)
    , _service(nullptr)
    , _impl()
    , _iface(nullptr) {
    LOGTRACE("App2AppProvider constructed (testing)");
}

App2AppProvider::~App2AppProvider() {
    LOGTRACE("App2AppProvider destructed (testing)");
}

Exchange::IAppGateway* App2AppProvider::AcquireGateway_(PluginHost::IShell* service) const {
    if (service == nullptr) {
        return nullptr;
    }
    // Prefer versioned callsign, fallback to unversioned if needed.
    Exchange::IAppGateway* gw = service->QueryInterfaceByCallsign<Exchange::IAppGateway>(_T("org.rdk.AppGateway"));
    if (gw == nullptr) {
        gw = service->QueryInterfaceByCallsign<Exchange::IAppGateway>(_T("AppGateway"));
    }
    return gw;
}

// PUBLIC_INTERFACE
const string App2AppProvider::Initialize(PluginHost::IShell* service) {
    LOGTRACE("Initialize enter (testing)");
    ASSERT(service != nullptr);
    _service = service;

    Exchange::IAppGateway* gateway = AcquireGateway_(_service);
    if (gateway == nullptr) {
        LOGERR("IAppGateway not available via IShell::QueryInterfaceByCallsign");
        return string("App2AppProvider(testing): AppGateway not available");
    }

    _impl.reset(new (std::nothrow) App2AppProviderImplementation(gateway));
    if (!_impl) {
        gateway->Release();
        LOGERR("Failed to allocate App2AppProviderImplementation");
        return string("App2AppProvider(testing): allocation failed");
    }

    _iface = static_cast<Exchange::IApp2AppProvider*>(_impl.get());
    LOGTRACE("Initialize exit (testing): success");
    return string();
}

// PUBLIC_INTERFACE
void App2AppProvider::Deinitialize(PluginHost::IShell* service) {
    LOGTRACE("Deinitialize enter (testing)");
    (void)service;

    if (_impl) {
        _impl.reset();
        _iface = nullptr;
    }

    _service = nullptr;
    LOGTRACE("Deinitialize exit (testing)");
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
