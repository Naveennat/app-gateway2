/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2022 RDK Management
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

#include <thread>

#include "PolyM/include/polym/Queue.hpp"
#include <functional>

namespace WPEFramework {
namespace Plugin       {

using Task = std::function<void()>;
using Request = std::function<bool()>;

class Processor
{
public:
    Processor();
    ~Processor();
    Processor(const Processor&) = delete;
    Processor& operator=(const Processor&) = delete;

    // Not blocking
    void scheduleTask(const Task& task);

    // Blocking
    bool request(const Request& request);

private:
    void mainLoop();

    enum class MsgType
    {
        Task = 1,
        Request,
        Response,
        EndLoop
    };

    PolyM::Queue queue_;
    std::thread mainThread_;
    bool keepWorking_{true};
};

} // namespace Plugin
} // namespace WPEFramework
