#pragma once

/*
 * Minimal RequestRouter implementation used by unit tests.
 * Echoes back the provided resolution and params into a response JSON object.
 */

#include <plugins/Module.h>
#include <core/JSON.h>
#include <string>

namespace WPEFramework {
namespace Plugin {

class RequestRouter {
public:
    // PUBLIC_INTERFACE
    RequestRouter(WPEFramework::PluginHost::IShell* service, const std::string& callsign)
        : _service(service), _callsign(callsign) {}

    // PUBLIC_INTERFACE
    uint32_t DispatchResolved(const Core::JSON::Object& resolution,
                              const Core::JSON::Object& params,
                              Core::JSON::Object& response);

private:
    WPEFramework::PluginHost::IShell* _service { nullptr };
    std::string _callsign;
};

} // namespace Plugin
} // namespace WPEFramework
