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

#include <atomic>
#include <functional>
#include <string>
#include <thread>
#include <unordered_map>

#include <sys/inotify.h>

namespace WPEFramework {
namespace Plugin       {

class FileSystemNotification
{
    using FileChangeCallback = std::function<void(const std::string &)>;

    struct DirectoryWatch
    {
        FileChangeCallback onFileCreationCallback;
        FileChangeCallback onFileDeletionCallback;
    };

public:
    FileSystemNotification();
    virtual ~FileSystemNotification();

    bool start();
    bool stop();

    bool addDirectoryWatch(const std::string &path,
        const FileChangeCallback &onFileCreationCallback,
        const FileChangeCallback &onFileDeletionCallback);

private:
    FileSystemNotification(const FileSystemNotification &) = delete;
    FileSystemNotification &operator=(const FileSystemNotification &) = delete;

    void clear();
    void loop();
    void handleDirectoryNotif(struct inotify_event *event);

    std::atomic_bool running_{false};
    std::thread loopThread_;
    int inotifyFd_{-1};
    std::unordered_map<int, DirectoryWatch> directoryWatches_;
};

} // namespace Plugin
} // namespace WPEFramework
