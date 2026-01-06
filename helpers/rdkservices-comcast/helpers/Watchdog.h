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

#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

#include "SafeConditionVariable.h"

namespace WPEFramework {
namespace Plugin       {

class Watchdog
{
public:
    Watchdog() = default;
    virtual ~Watchdog() = default;

    bool start(unsigned timeout, std::function<void()> onTimerExpired);
    bool stop();
    bool restart();

private:
    Watchdog(const Watchdog&) = delete;
    Watchdog& operator=(const Watchdog&) = delete;

    void clear();
    void loop();
    void setCurrentTime();
    bool isTimerExpired();

    std::atomic_bool running_{false};
    std::mutex lastTimeMutex_;
    std::chrono::time_point<std::chrono::system_clock> lastTime_;
    std::thread loopThread_;
    SafeConditionVariable safeConditionVariable_;
    std::chrono::milliseconds timeout_{0};
    std::function<void()> onTimerExpired_;
};

} // namespace Plugin
} // namespace WPEFramework
