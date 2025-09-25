#pragma once

// Module.h: Common module header for the AppGateway plugin.
// Follows Thunder (WPEFramework) plugin conventions to define module metadata
// and export macros.

#include <plugins/Module.h>

#undef EXTERNAL
#ifdef _WIN32
#ifdef AppGateway_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#endif
#else
#define EXTERNAL
#endif

// Define module name consistent with Thunder macros
#ifndef MODULE_NAME
#define MODULE_NAME AppGateway
#endif

// Versioning for the plugin (major.minor.patch)
#define APPGATEWAY_VERSION_MAJOR 1
#define APPGATEWAY_VERSION_MINOR 0
#define APPGATEWAY_VERSION_PATCH 0

