/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2026
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

#include <plugins/Module.h>

#ifndef ID_APP_NOTIFICATIONS
// Keep consistent with ids used in interfaces/Ids.h in this repository.
// (If an upstream build provides different values, the SDK headers would
// take precedence via include paths.)
#define ID_APP_NOTIFICATIONS (RPC::IDS::ID_EXTERNAL_INTERFACE_OFFSET + 0x2000 + 0x020)
#endif

namespace WPEFramework {
namespace Exchange {

/*
 * Minimal interface surface required by the AppGateway plugin in this repository.
 * The plugin only uses Notify(eventName, payload) for best-effort signaling.
 */
struct EXTERNAL IAppNotifications : virtual public Core::IUnknown {
    enum { ID = ID_APP_NOTIFICATIONS };

    // Notify a consumer (e.g., AppNotifications plugin) of an event with a payload.
    // payload is treated as an opaque string by callers.
    virtual Core::hresult Notify(const string& eventName /* @in */,
                                 const string& payload /* @in @opaque */) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
