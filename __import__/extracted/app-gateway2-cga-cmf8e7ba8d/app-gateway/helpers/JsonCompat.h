#pragma once

// Compatibility layer to map Core::JSON::Object to VariantContainer
// for environments where WPEFramework/core/JSON.h does not define Object.
#include <core/JSON.h>

namespace WPEFramework {
namespace Core {
namespace JSON {

// Provide compatibility alias so existing code using Core::JSON::Object works.
using Object = VariantContainer;

} // namespace JSON
} // namespace Core
} // namespace WPEFramework
