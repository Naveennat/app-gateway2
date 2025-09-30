#include "OttServicesImplementation.h"

#include <core/Enumerate.h>

namespace WPEFramework {
namespace Plugin {

    OttServicesImplementation::OttServicesImplementation()
        : _service(nullptr)
        , _state(_T("uninitialized")) {
    }

    OttServicesImplementation::~OttServicesImplementation() = default;

    string OttServicesImplementation::Initialize(PluginHost::IShell* service) {
        _service = service;
        _state = _T("ready");
        // Return empty string on success per Thunder conventions.
        return string();
    }

    void OttServicesImplementation::Deinitialize(PluginHost::IShell* service) {
        // Clean up any resources here.
        _state = _T("deinitialized");
        _service = nullptr;
    }

    uint32_t OttServicesImplementation::Ping(const string& message, string& reply) {
        // Simple echo behavior; actual behavior should follow spec if different.
        reply = (message.empty() ? string(_T("pong")) : (string(_T("pong: ")) + message));
        return Core::ERROR_NONE;
    }

} // namespace Plugin
} // namespace WPEFramework
