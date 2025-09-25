#include "RequestRouter.h"

namespace WPEFramework {
namespace Plugin {

// Basic placeholder. In a full implementation this would use JSONRPC direct link
// to invoke target plugin methods with the security token.
uint32_t RequestRouter::DispatchResolved(const Core::JSON::Object& resolution,
                                         const Core::JSON::Object& callParams,
                                         Core::JSON::Object& response) {
    (void)_service;
    (void)_securityToken;

    // Default "echo" behavior for now: return resolution and params.
    Core::JSON::Object out;
    out.Set(_T("resolution"), resolution);
    out.Set(_T("params"), callParams);
    response = out;

    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
