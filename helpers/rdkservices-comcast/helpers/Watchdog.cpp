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

#include "Watchdog.h"

#include "utils.h"

namespace WPEFramework {
namespace Plugin       {

bool Watchdog::start(unsigned timeout, std::function<void()> onTimerExpired)
{
    LOGINFO("Going to start watchdog with %u ms timer", timeout);

    if (running_)
    {
        LOGERR("Watchdog is already running");
        return false;
    }

    clear();

    running_ = true;
    timeout_ = std::chrono::milliseconds{timeout};
    onTimerExpired_ = onTimerExpired;

    LOGINFO("Starting watchdog with %llu ms timer", timeout_.count());

    setCurrentTime();
    loopThread_ = std::thread(&Watchdog::loop, this);

    return true;
}

/**
 * It ensures ongoing thread is joined before the new thread is started.
 * It would happen if Watchdog::stop() was not called.
 **/
void Watchdog::clear()
{
    if (loopThread_.joinable())
    {
        LOGINFO("Ongoing loop thread needs to be joined firstly");
        loopThread_.join();
    }
}

void Watchdog::setCurrentTime()
{
    std::lock_guard<std::mutex> lock(lastTimeMutex_);
    lastTime_ = std::chrono::system_clock::now();
}

void Watchdog::loop()
{
    while (running_ && !isTimerExpired())
    {
        safeConditionVariable_.waitFor(timeout_);
    }

    if (running_)
    {
        running_ = false;
        onTimerExpired_();
    }
}

bool Watchdog::isTimerExpired()
{
    std::lock_guard<std::mutex> lock(lastTimeMutex_);
    return (std::chrono::system_clock::now() - lastTime_) >= timeout_;
}

bool Watchdog::stop()
{
    LOGINFO("Going to stop currently running watchdog");

    if (!running_)
    {
        LOGERR("Watchdog is not running now");
        return false;
    }

    running_ = false;

    safeConditionVariable_.notifyAll();
    if (loopThread_.joinable())
    {
        loopThread_.join();
    }

    return true;
}

bool Watchdog::restart()
{
    LOGINFO("Going to restart timer for currently running watchdog");

    if (!running_)
    {
        LOGERR("Watchdog is not running now");
        return false;
    }

    setCurrentTime();
    safeConditionVariable_.notifyAll();

    return true;
}

} // namespace Plugin
} // namespace WPEFramework
