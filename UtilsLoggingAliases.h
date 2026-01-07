#pragma once
/*
 * Logging compatibility macros for this isolated build.
 *
 * Upstream Thunder provides LOG_* macros but signatures differ; implement
 * variadic no-op macros matching the call-sites in this repo.
 */

#ifndef LOGINFO
#define LOGINFO(...) do { } while(0)
#endif

#ifndef LOGERR
#define LOGERR(...) do { } while(0)
#endif

#ifndef LOGWARN
#define LOGWARN(...) do { } while(0)
#endif

#ifndef LOGDBG
#define LOGDBG(...) do { } while(0)
#endif

#ifndef LOGTRACE
#define LOGTRACE(...) do { } while(0)
#endif
