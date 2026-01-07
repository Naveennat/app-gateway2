#pragma once
/*
 * Minimal shim for legacy include layout: <plugins/Module.h>
 *
 * This isolated repository build must not include the full Thunder SDK Module.h,
 * because it pulls in the websocket stack which is incompatible with the
 * vendored Core headers in this environment.
 *
 * Provide only the minimal symbols that this repo's headers expect:
 *  - MODULE_NAME macro (compile definition supplies actual value)
 *  - EXTERNAL/EXTERNAL_REFERENCE compatibility (often from interfaces/definitions.h)
 *  - Interface-map macros as no-ops (used in plugin headers for registration)
 */

#include "../interfaces/definitions.h"
#include "../UtilsLoggingAliases.h"

/* Interface map macros used in AppGateway headers; no-op for isolated build. */
#ifndef BEGIN_INTERFACE_MAP
#define BEGIN_INTERFACE_MAP(...) /* no-op */
#endif
#ifndef END_INTERFACE_MAP
#define END_INTERFACE_MAP /* no-op */
#endif
#ifndef INTERFACE_ENTRY
#define INTERFACE_ENTRY(...) /* no-op */
#endif
#ifndef INTERFACE_AGGREGATE
#define INTERFACE_AGGREGATE(...) /* no-op */
#endif

/* Service registration macro; no-op for isolated build. */
#ifndef SERVICE_REGISTRATION
#define SERVICE_REGISTRATION(...) /* no-op */
#endif

#ifndef MODULE_NAME_DECLARATION
#define MODULE_NAME_DECLARATION(...) /* no-op */
#endif
