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

#include "SafeConditionVariable.h"

#include "utils.h"

namespace WPEFramework {
namespace Plugin       {

void SafeConditionVariable::notifyAll()
{
    LOGINFO("Going to notify currently waiting threads");

    std::lock_guard<std::mutex> lock(mutex_);
    ready_ = true;
    condition_.notify_all();
}

void SafeConditionVariable::waitFor(const std::chrono::milliseconds& timeout)
{
    LOGINFO("Going to block current thread for %llu ms", timeout.count());

    std::unique_lock<std::mutex> lock(mutex_);
    ready_ = false;
    condition_.wait_for(lock, timeout, [this]() { return ready_; });
}

} // namespace Plugin
} // namespace WPEFramework
