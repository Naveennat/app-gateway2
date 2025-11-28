/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2021 RDK Management
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
**/
#pragma once

#include "Module.h"
#include <interfaces/Ids.h>

namespace WPEFramework
{
namespace Exchange
{

/* TODO:
 *  Generate Thunder proxy stub to be able to use it as IPC proxy
 *  Thunder proxy stub supports only basic types (int, string, bool).
 *  Usage in scope of single WPE process is fine without proxy stub
 */

struct EXTERNAL IDeviceBridgeProxy : virtual public Core::IUnknown
{
    enum
    {
        ID = ID_BROWSER + 0x20000
    };

    struct INotification : virtual public Core::IUnknown
    {
        enum
        {
            ID = IDeviceBridgeProxy::ID + 1
        };

        virtual ~INotification() {}
        virtual void Event(const string &eventName, const string &data) const = 0;
    };

    virtual ~IDeviceBridgeProxy() {}

    virtual void RegisterEventsObserver(
        const Exchange::IDeviceBridgeProxy::INotification &sink,
        const std::vector<std::string> &eventNames) = 0;
    virtual void UnregisterEventsObserver(const Exchange::IDeviceBridgeProxy::INotification &sink) = 0;
    virtual uint32_t SendDeviceSpecificCmd(const string &input, string &output /* @out */) = 0;
};

} // namespace Exchange
} // namespace WPEFramework