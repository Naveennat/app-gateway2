#pragma once

// Common module header for the App2AppProvider plugin.
// Follows Thunder (WPEFramework) plugin conventions to define module metadata
// and export macros.

#include <plugins/Module.h>

#undef EXTERNAL
#ifdef _WIN32
#ifdef App2AppProvider_EXPORTS
#define EXTERNAL __declspec(dllexport)
#else
#define EXTERNAL __declspec(dllimport)
#endif
#else
#define EXTERNAL
#endif

#ifndef MODULE_NAME
#define MODULE_NAME App2AppProvider
#endif

// Versioning for the plugin (major.minor.patch)
#define APP2APPPROVIDER_VERSION_MAJOR 1
#define APP2APPPROVIDER_VERSION_MINOR 0
#define APP2APPPROVIDER_VERSION_PATCH 0
