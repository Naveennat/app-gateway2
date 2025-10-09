#pragma once

// Module foundation header for the OttServices plugin.
// Sets up the module name, Thunder (WPEFramework) includes, and interface IDs.

#ifndef MODULE_NAME
#define MODULE_NAME OttServices
#endif

#include <core/core.h>
#include <plugins/IPlugin.h>
#include <plugins/JSONRPC.h>
#include <plugins/IShell.h>

#ifndef EXTERNAL
#ifdef __WINDOWS__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL
#endif
#endif

// Assign a unique interface ID for IOttServices. Keep stable across builds.
#ifndef ID_OTT_SERVICES
#define ID_OTT_SERVICES 0x0000F812
#endif

#ifndef ID_OTT_PERMISSION_CACHE
#define ID_OTT_PERMISSION_CACHE 0x0000F813
#endif

// Compatibility alias: some Thunder/WPEFramework versions use VariantContainer as the dynamic JSON object.
// Provide an alias so Object is always available in code that uses it.
namespace WPEFramework {
namespace Core {
namespace JSON {
    using Object = VariantContainer;
}
}
}
