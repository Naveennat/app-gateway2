#pragma once

// Module foundation header for this plugin.
// This mirrors the pattern used by Metrological/Thunder plugins.

#ifndef MODULE_NAME
#define MODULE_NAME AppGateway
#endif

#include <core/core.h>
#include <plugins/JSONRPC.h>
#include <plugins/IPlugin.h>
#include <plugins/IShell.h>

#ifndef EXTERNAL
#ifdef __WINDOWS__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL
#endif
#endif

 // Convenience macro used by Thunder for version and module declaration.
 // Keep the declaration in Module.cpp via MODULE_NAME_DECLARATION(BUILD_REFERENCE).

// Backwards-compatibility alias:
//
// Some Thunder/WPEFramework versions do not define Core::JSON::Object.
// They instead provide Core::JSON::VariantContainer for dynamic objects.
// Create an alias so code using Core::JSON::Object continues to compile.
namespace WPEFramework {
namespace Core {
namespace JSON {
    using Object = VariantContainer;
}
}
}
