#pragma once
/*
 * Minimal <tracing/tracing.h> shim for this repository's isolated AppGateway build.
 *
 * The full WPEFramework tracing headers depend on messaging infrastructure that
 * is not available/compatible in this isolated build setup. AppGateway only
 * requires basic TRACE_Lx macros to compile; implement them as no-ops here.
 */

#ifndef TRACE_L1
#define TRACE_L1(...) do { } while(0)
#endif
#ifndef TRACE_L2
#define TRACE_L2(...) do { } while(0)
#endif
#ifndef TRACE_L3
#define TRACE_L3(...) do { } while(0)
#endif
#ifndef TRACE_L4
#define TRACE_L4(...) do { } while(0)
#endif
#ifndef TRACE_L5
#define TRACE_L5(...) do { } while(0)
#endif
