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

// Default gRPC timeout settings (mirrors CloudStore behavior).
// GRPC_TIMEOUT: per-RPC deadline (milliseconds).
// IDLE_TIMEOUT: gRPC channel idle timeout (milliseconds).
#ifndef GRPC_TIMEOUT
#define GRPC_TIMEOUT (5000)
#endif

#ifndef IDLE_TIMEOUT
#define IDLE_TIMEOUT (300000)
#endif

#undef EXTERNAL
#define EXTERNAL
