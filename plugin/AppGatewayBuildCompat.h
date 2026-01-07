#pragma once
/*
 * Build-compat header force-included for the isolated AppGateway build.
 *
 * Provides:
 *  - Logging macro aliases (LOGINFO/LOGERR) used by AppGateway sources.
 *  - No-op stubs for Thunder plugin metadata & service registration macros to
 *    avoid SDK version interface mismatches in MetaData.h.
 */

#include "../UtilsLoggingAliases.h"
#include "../ErrorUtils.h"

#ifndef SERVICE_REGISTRATION
#define SERVICE_REGISTRATION(...) /* no-op */
#endif

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

// Callsign constants used by AppGateway sources in upstream builds.
#ifndef APP_NOTIFICATIONS_CALLSIGN
#define APP_NOTIFICATIONS_CALLSIGN "org.rdk.AppNotifications"
#endif
#ifndef INTERNAL_GATEWAY_CALLSIGN
#define INTERNAL_GATEWAY_CALLSIGN "org.rdk.AppGateway"
#endif
#ifndef APP_GATEWAY_CALLSIGN
#define APP_GATEWAY_CALLSIGN "org.rdk.AppGateway"
#endif
#ifndef GATEWAY_AUTHENTICATOR_CALLSIGN
#define GATEWAY_AUTHENTICATOR_CALLSIGN "org.rdk.AppGatewayAuthenticator"
#endif
