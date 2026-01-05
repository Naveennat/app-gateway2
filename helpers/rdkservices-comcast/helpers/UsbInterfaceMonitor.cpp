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

#include "UsbInterfaceMonitor.h"

#include <string>
#include <unistd.h>

#include "utils.h"

namespace WPEFramework
{
namespace Plugin
{

namespace
{

const std::string TMP_CHERRY_PATH = "/tmp/cherry";
const std::string CHERRY_INTERFACE_DETECTED = "cherry_interface_detected";

bool exist(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}

} // namespace

UsbInterfaceMonitor::UsbInterfaceMonitor()
{
    if (not exist(TMP_CHERRY_PATH))
    {
        if (mkdir(TMP_CHERRY_PATH.c_str(), 0644) < 0)
        {
            LOGERR("Cannot create %s directory: %s", TMP_CHERRY_PATH.c_str(), strerror(errno));
        }
    }

    fileSystemNotification.addDirectoryWatch(TMP_CHERRY_PATH,
        [this](const std::string &fileName)
        {
            if (fileName == CHERRY_INTERFACE_DETECTED)
            {
                notifyUsbInterfaceDetectedChange(true);
            }
        },
        [this](const std::string &fileName)
        {
            if (fileName == CHERRY_INTERFACE_DETECTED)
            {
                notifyUsbInterfaceDetectedChange(false);
            }
        });
}

void UsbInterfaceMonitor::initialize(const OnUsbInterfaceChangeCallback& onChange)
{
    onChange_ = onChange;
}

void UsbInterfaceMonitor::deinitialize()
{
    stopMonitoring();
    onChange_ = boost::none;
}

void UsbInterfaceMonitor::startMonitoring()
{
    if (exist(std::string{TMP_CHERRY_PATH + '/' + CHERRY_INTERFACE_DETECTED}))
    {
        notifyUsbInterfaceDetectedChange(true);
    }

    fileSystemNotification.start();
}

void UsbInterfaceMonitor::notifyUsbInterfaceDetectedChange(bool isDetected)
{
    if (boost::none != onChange_)
    {
        LOGINFO("USB interface dectected change: %s", isDetected ? "true" : "false");
        onChange_.get()(isDetected);
    }
}

void UsbInterfaceMonitor::stopMonitoring()
{
    fileSystemNotification.stop();
}

} // namespace Plugin
} // namespace WPEFramework
