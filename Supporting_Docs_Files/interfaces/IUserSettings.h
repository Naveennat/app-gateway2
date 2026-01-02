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

#ifndef ID_USER_SETTINGS
#define ID_USER_SETTINGS 0xFA200107
#endif

namespace WPEFramework {
namespace Exchange {

struct EXTERNAL IUserSettings : virtual public Core::IUnknown {
    enum { ID = ID_USER_SETTINGS };

    virtual Core::hresult Get(const string& key /* @in */, string& value /* @out @opaque */) = 0;
    virtual Core::hresult Set(const string& key /* @in */, const string& value /* @in @opaque */) = 0;
};

} // namespace Exchange
} // namespace WPEFramework
