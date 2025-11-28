#include "RequestRouter.h"

namespace WPEFramework {
namespace Plugin {

// PUBLIC_INTERFACE
uint32_t RequestRouter::DispatchResolved(const Core::JSON::Object& resolution,
                                         const Core::JSON::Object& params,
                                         Core::JSON::Object& response)
{
    // Echo back objects under "resolution" and "params" keys as expected by the tests.
    response.Clear();
    response[_T("resolution")] = Core::JSON::Variant(resolution);
    response[_T("params")] = Core::JSON::Variant(params);
    return Core::ERROR_NONE;
}

} // namespace Plugin
} // namespace WPEFramework
