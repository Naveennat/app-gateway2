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

#include <plugins/Module.h>

#ifndef ID_FB_METRICS
#define ID_FB_METRICS 0xFA200103
#endif

namespace WPEFramework {
namespace Exchange {

struct EXTERNAL IFbMetrics : virtual public Core::IUnknown {
    enum { ID = ID_FB_METRICS };

    virtual Core::hresult PostMetric(const string& payload /* @in @opaque */) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
