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

#undef EXTERNAL
#define EXTERNAL
