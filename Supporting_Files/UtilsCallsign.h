#pragma once

// Central callsign constants used across AppGateway sources.
// Guard each macro to avoid redefinition when building with entservices-infra helpers.

#ifndef APP_GATEWAY_CALLSIGN
#define APP_GATEWAY_CALLSIGN "org.rdk.AppGateway"
#endif

#ifndef APP_NOTIFICATIONS_CALLSIGN
#define APP_NOTIFICATIONS_CALLSIGN "org.rdk.AppNotifications"
#endif

#ifndef INTERNAL_GATEWAY_CALLSIGN
#define INTERNAL_GATEWAY_CALLSIGN "org.rdk.LaunchDelegate"
#endif

#ifndef GATEWAY_AUTHENTICATOR_CALLSIGN
#define GATEWAY_AUTHENTICATOR_CALLSIGN "org.rdk.LaunchDelegate"
#endif

#ifndef APP_TO_APP_PROVIDER_CALLSIGN
#define APP_TO_APP_PROVIDER_CALLSIGN "org.rdk.App2AppProvider"
#endif

#ifndef FB_PRIVACY_CALLSIGN
#define FB_PRIVACY_CALLSIGN "org.rdk.FbPrivacy"
#endif

#ifndef FB_METRICS_CALLSIGN
#define FB_METRICS_CALLSIGN "org.rdk.FbMetrics"
#endif

#ifndef ANALYTICS_PLUGIN_CALLSIGN
#define ANALYTICS_PLUGIN_CALLSIGN "org.rdk.Analytics"
#endif
