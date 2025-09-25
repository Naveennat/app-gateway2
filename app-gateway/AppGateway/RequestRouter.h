#pragma once

#include "Module.h"

namespace WPEFramework {
namespace Plugin {

/**
 * RequestRouter
 * Bridges resolution output to local Thunder plugin method invocation.
 * Placeholder implementation – to be extended with JSONRPCDirectLink usage.
 */
class RequestRouter {
public:
    explicit RequestRouter(PluginHost::IShell* service, const string& securityToken)
        : _service(service)
        , _securityToken(securityToken) {
    }

    // PUBLIC_INTERFACE
    uint32_t DispatchResolved(const Core::JSON::Object& resolution,
                              const Core::JSON::Object& callParams,
                              Core::JSON::Object& response);

private:
    PluginHost::IShell* _service;
    string _securityToken;
};

} // namespace Plugin
} // namespace WPEFramework
