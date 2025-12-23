#pragma once

/*
 PUBLIC_INTERFACE
 Minimal shim header to satisfy includes expecting "UtilsJsonrpcDirectLink.h".
 Several modules use macros and helpers from UtilsJsonRpc.h to simplify JSON-RPC calls.
 This header forwards to UtilsJsonRpc.h if available, otherwise defines minimal macros.

 Preference order:
 1) Include entservices-infra UtilsJsonRpc if available via include paths
 2) Provide fallback minimal macro definitions used in this repo
*/

#if __has_include("UtilsJsonRpc.h")
#include "UtilsJsonRpc.h"
#elif __has_include("UtilsJsonrpc.h")
#include "UtilsJsonrpc.h"
#else
// Fallback minimal macros used by Resolver.cpp and others
#include <core/JSON.h>
#include <string>
#include <cstdio>

#ifndef LOGERR
#define LOGERR(fmt, ...) do { std::fprintf(stderr, "ERROR: " fmt "\n", ##__VA_ARGS__); } while(0)
#endif
#ifndef LOGWARN
#define LOGWARN(fmt, ...) do { std::fprintf(stderr, "WARN: " fmt "\n", ##__VA_ARGS__); } while(0)
#endif
#ifndef LOGINFO
#define LOGINFO(fmt, ...) do { std::fprintf(stderr, "INFO: " fmt "\n", ##__VA_ARGS__); } while(0)
#endif

#endif
