#pragma once

// Module foundation header for App2AppProvider plugin (testing tree).
// Defines the module name and pulls in Thunder (WPEFramework) essentials.

#ifndef MODULE_NAME
#define MODULE_NAME App2AppProvider
#endif

#include <core/Core.h>
#include <plugins/IPlugin.h>
#include <plugins/IShell.h>

#ifndef EXTERNAL
#ifdef __WINDOWS__
#define EXTERNAL EXTERNAL_EXPORT
#else
#define EXTERNAL
#endif
#endif

// Provide stable interface IDs if not present via build system.
#ifndef ID_APP2APP_PROVIDER
#define ID_APP2APP_PROVIDER 0x0000F810
#endif

#ifndef ID_APP_GATEWAY
#define ID_APP_GATEWAY 0x0000F811
#endif

namespace WPEFramework {
namespace Core {
namespace JSON {
    using Object = VariantContainer;
}
}
}
