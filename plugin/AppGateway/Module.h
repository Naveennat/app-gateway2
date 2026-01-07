/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#pragma once
#ifndef MODULE_NAME
#define MODULE_NAME Plugin_AppGateway
#endif

#include <core/core.h>
#include <plugins/IPlugin.h>
#include <plugins/IShell.h>
#include <tracing/tracing.h>
#include <core/Services.h>

// Ensure the isolated repo's logging macro shims are available even when the build
// does not inject helper include directories (e.g., when building via app-gateway/CMakeLists.txt).
#include <UtilsLoggingAliases.h>

// Ensure "ErrorUtils::..." symbols referenced by implementation files are available.
// (Repo provides a forwarding header at app-gateway2/ErrorUtils.h to the plugin stub.)
#include <ErrorUtils.h>

// Callsign constants expected by AppGatewayImplementation.cpp / ResponderImplementation.cpp.
// In upstream builds these come from platform-specific headers/config.
#ifndef APP_NOTIFICATIONS_CALLSIGN
#define APP_NOTIFICATIONS_CALLSIGN "org.rdk.AppNotifications"
#endif

#ifndef INTERNAL_GATEWAY_CALLSIGN
#define INTERNAL_GATEWAY_CALLSIGN "org.rdk.AppGateway"
#endif

 // Pull in the repo's interface IDs (e.g., ID_APP_GATEWAY*) via normal include paths.
 // The build config adds app-gateway2/interfaces to the include dirs, so this resolves to
 // app-gateway2/interfaces/Ids.h (not the SDK one).
#include <Ids.h>
