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

#include "FileSystemNotification.h"

#include "utils.h"

namespace WPEFramework {
namespace Plugin       {

FileSystemNotification::FileSystemNotification()
{
    inotifyFd_ = inotify_init();
    if (inotifyFd_ < 0)
    {
        LOGWARN("An error happened during inotify initialization");
    }
}

FileSystemNotification::~FileSystemNotification()
{
    if (running_)
    {
        stop();
    }

    clear();
    if (close(inotifyFd_) < 0)
    {
        LOGWARN("Cannot close inotify file descriptor: %s", strerror(errno));
    }
}

bool FileSystemNotification::stop()
{
    if (not running_)
    {
        LOGWARN("This instance is not running now");
        return false;
    }

    running_ = false;
    if (loopThread_.joinable())
    {
        loopThread_.join();
    }

    return true;
}

void FileSystemNotification::clear()
{
    for (const auto &pair : directoryWatches_)
    {
        if (inotify_rm_watch(inotifyFd_, pair.first) < 0)
        {
            LOGWARN("Cannot remove a directory watch: %s", strerror(errno));
        }
    }

    directoryWatches_.clear();
}

bool FileSystemNotification::start()
{
    if (running_)
    {
        LOGWARN("This instance is already running");
        return false;
    }

    running_ = true;
    loopThread_ = std::thread(&FileSystemNotification::loop, this);

    return true;
}

void FileSystemNotification::loop()
{
    while (running_)
    {
        constexpr unsigned size = sizeof(inotify_event) + NAME_MAX + 1;
        char buffer[size];

        const int length = read(inotifyFd_, buffer, size);
        if (length <= 0)
        {
            LOGWARN("An error happened when reading from inotify file descriptor: %s", strerror(errno));
            continue;
        }

        int currentPtr = 0;
        while (currentPtr < length)
        {
            inotify_event *event = reinterpret_cast<inotify_event *>(&buffer[currentPtr]);
            handleDirectoryNotif(event);
            currentPtr += sizeof(inotify_event) + event->len;
        }
    }
}

void FileSystemNotification::handleDirectoryNotif(inotify_event *event)
{
    const auto it = directoryWatches_.find(event->wd);
    if (it == directoryWatches_.end())
    {
        LOGWARN("Received an unknown event");
        return;
    }

    const std::string filename = event->len ? event->name : "";
    if (filename.empty())
    {
        LOGWARN("Event with empty name received");
        return;
    }

    if (event->mask & IN_CREATE)
    {
        it->second.onFileCreationCallback(filename);
    }
    else if (event->mask & IN_DELETE)
    {
        it->second.onFileDeletionCallback(filename);
    }
    else if (event->mask & IN_IGNORED)
    {
        LOGWARN("Received IN_IGNORED event - watch was removed explicitly or automatically");
        if (inotify_rm_watch(inotifyFd_, it->first) < 0)
        {
            LOGWARN("Cannot remove a directory watch: %s", strerror(errno));
        }
        directoryWatches_.erase(it);
    }
}

bool FileSystemNotification::addDirectoryWatch(const std::string &path,
    const FileChangeCallback &onFileCreationCallback,
    const FileChangeCallback &onFileDeletionCallback)
{
    LOGINFO("Going to observe created and removed files in %s", path.c_str());

    if (onFileCreationCallback == nullptr || onFileDeletionCallback == nullptr)
    {
        LOGWARN("Cannot create a directory watch without callbacks");
        return false;
    }

    const int watchFd = inotify_add_watch(inotifyFd_, path.c_str(), IN_CREATE | IN_DELETE);
    if (watchFd < 0)
    {
        LOGWARN("Cannot create a directory watch: %s", strerror(errno));
        return false;
    }

    DirectoryWatch watch;
    watch.onFileCreationCallback = onFileCreationCallback;
    watch.onFileDeletionCallback = onFileDeletionCallback;

    directoryWatches_.insert(std::make_pair(watchFd, watch));

    return true;
}

} // namespace Plugin
} // namespace WPEFramework
