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

// NOTE:
// Do NOT override Thunder/WPEFramework core macros like SERVICE_REGISTRATION or the
// interface-map macros (BEGIN_INTERFACE_MAP/INTERFACE_ENTRY/etc). They are required
// by the SDK headers and by the plugin implementation headers in this repository.
// Overriding them results in hard-to-debug parse errors in SDK headers (e.g. VirtualInput.h)
// and breaks interface declarations.
//
// This compat header should only provide missing shims that are genuinely absent in the
// isolated build environment (logging aliases, ErrorUtils, and callsign constants).

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
