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

#include "Processor.h"

#include "utils.h"

namespace WPEFramework {
namespace Plugin       {

Processor::Processor()
{
    LOGINFO("Starting main loop.");
    mainThread_ = std::thread(&Processor::mainLoop, this);
}

Processor::~Processor()
{
    LOGINFO("Stopping main loop.");
    keepWorking_ = false;
    queue_.put(PolyM::Msg((int)MsgType::EndLoop));
    if (mainThread_.joinable())
    {
        mainThread_.join();
    }
    LOGINFO("Main thread joined.");
}

void Processor::scheduleTask(const Task& task)
{
    auto&& taskMsg = PolyM::DataMsg<Task>((int)MsgType::Task, task);
    LOGINFO("Scheduling a task with id %lli.", taskMsg.getUniqueId());
    queue_.put(std::move(taskMsg));
}

bool Processor::request(const Request& request)
{
    auto&& requestMsg = PolyM::DataMsg<Request>((int)MsgType::Request, request);
    const auto& requestId = requestMsg.getUniqueId();

    LOGINFO("Starting request with id %lli.", requestId);
    const auto& responseMsg = queue_.request(std::move(requestMsg));
    LOGINFO("Received response with id: %lli to request that had id: %lli", responseMsg->getUniqueId(), requestId);

    const auto& response = dynamic_cast<PolyM::DataMsg<bool>&>(*responseMsg);
    return response.getPayload();
}

void Processor::mainLoop()
{
    while (keepWorking_)
    {
        const auto& msg = queue_.get();
        switch (MsgType(msg->getMsgId()))
        {
            case MsgType::Task:
            {
                LOGINFO("Executing task with id: %lli", msg->getUniqueId());
                const auto& task = dynamic_cast<PolyM::DataMsg<Task>&>(*msg);
                task.getPayload()();
                LOGINFO("Finished executing task with id: %lli", msg->getUniqueId());
                break;
            }
            case MsgType::Request:
            {
                LOGINFO("Handling request with id: %lli", msg->getUniqueId());
                const auto& request = dynamic_cast<PolyM::DataMsg<Request>&>(*msg);
                const auto& result = request.getPayload()();

                auto&& responseMsg = PolyM::DataMsg<bool>((int)MsgType::Response, result);
                LOGINFO("Responding to request with id: %lli with response that has id: %lli",
                    msg->getUniqueId(), responseMsg.getUniqueId());

                const auto& responseResult = queue_.respondTo(msg->getUniqueId(), std::move(responseMsg));
                if (!responseResult)
                {
                    LOGERR("Error while responding to request with id: %lli", msg->getUniqueId());
                }
                LOGINFO("Finished handling request with id: %lli", msg->getUniqueId());
                break;
            }
            case MsgType::Response:
            {
                LOGERR("Response shouldn't be processed by main loop.");
                break;
            }
            case MsgType::EndLoop:
            {
                LOGINFO("Received end loop message.");
                return;
            }
        }
    }
    LOGINFO("Main loop finished.");
}

} // namespace Plugin
} // namespace WPEFramework
