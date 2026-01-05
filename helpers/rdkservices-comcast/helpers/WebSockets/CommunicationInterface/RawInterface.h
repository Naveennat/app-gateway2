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
#include <string>

namespace WPEFramework {
namespace Plugin       {
namespace WebSockets   {

template<typename Derived>
class RawInterface
{
public:
    RawInterface() = default;

    bool sendCommand(std::string command)
    {
        LOGINFO("Send command: %s", command.c_str());
        Derived& derived = static_cast<Derived&>(*this);
        return derived.send(command);
    }

protected:
    ~RawInterface() = default;

    void onMessage(const std::string& message)
    {
        LOGINFO("On message: %s", message.c_str());
    }

private:
    RawInterface(const RawInterface&) = delete;
    RawInterface& operator=(const RawInterface&) = delete;
};

}   // namespace Websockets
}   // namespace Plugin
}   // namespace WPEFramework