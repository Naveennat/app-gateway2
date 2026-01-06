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

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace WPEFramework {
namespace Plugin       {

/**
 * This class wraps std::condition_variable to mitigate issues known from the
 * standard library:
 * - Lost Wakeup and Spurious Wakeup - to avoid these two issues, an additional
 * predicate (as memory) needs to be used. This is the reason why extra boolean
 * field needs to be checked when std::condition_variable::wait_for is called.
 * - An atomic predicate - std::condition_variable::wait_for is more complicated
 * that it seems to be. The expression is equivalent to few lines. It could
 * happen notification is sent while the condition variable is in the wait_for
 * expression but not in the waiting state. Thus, even if boolean field was
 * atomic, a race condition would still happen. This is the reason why boolean
 * field needs to be modified under the mutex.
 *
 * All requirements, notes, exceptions etc. valid for std::condition_variable
 * are valid for this class too.
 **/
class SafeConditionVariable final
{
public:
    SafeConditionVariable() = default;
    ~SafeConditionVariable() = default;

    void notifyAll();
    void waitFor(const std::chrono::milliseconds& timeout);

private:
    SafeConditionVariable(const SafeConditionVariable&) = delete;
    SafeConditionVariable& operator=(const SafeConditionVariable&) = delete;

    SafeConditionVariable(const SafeConditionVariable&&) = delete;
    SafeConditionVariable& operator=(const SafeConditionVariable&&) = delete;

    bool ready_{false};
    std::mutex mutex_;
    std::condition_variable condition_;
};

} // namespace Plugin
} // namespace WPEFramework
