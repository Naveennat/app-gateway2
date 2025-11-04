#pragma once

#include <cstdio>

#ifndef LOGTRACE
#define LOGTRACE(fmt, ...) std::printf("[TRACE] " fmt "\n", ##__VA_ARGS__)
#endif

#ifndef LOGINFO
#define LOGINFO(fmt, ...)  std::printf("[INFO ] " fmt "\n", ##__VA_ARGS__)
#endif

#ifndef LOGWARN
#define LOGWARN(fmt, ...)  std::printf("[WARN ] " fmt "\n", ##__VA_ARGS__)
#endif

#ifndef LOGERR
#define LOGERR(fmt, ...)   std::fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#endif
