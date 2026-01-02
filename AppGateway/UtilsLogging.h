#pragma once

/*
 * Minimal logging helpers for standalone/unit test builds.
 *
 * The production environment typically provides richer logging helpers via
 * platform layers. For this repository's standalone L0 tests, we only need
 * the LOG* macros used across AppGateway sources to compile and run.
 */

#include <cstdio>

#ifndef LOGINFO
#define LOGINFO(fmt, ...)  std::fprintf(stderr, "[INFO] " fmt "\n", ##__VA_ARGS__)
#endif

#ifndef LOGWARN
#define LOGWARN(fmt, ...)  std::fprintf(stderr, "[WARN] " fmt "\n", ##__VA_ARGS__)
#endif

#ifndef LOGERR
#define LOGERR(fmt, ...)   std::fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif

#ifndef LOGTRACE
#define LOGTRACE(fmt, ...) std::fprintf(stderr, "[TRACE] " fmt "\n", ##__VA_ARGS__)
#endif
