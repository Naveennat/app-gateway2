/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2024 RDK Management
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
*/

#include "LifecycleManagement.h"

namespace WPEFramework {

    ENUM_CONVERSION_BEGIN(Plugin::LifecycleState)
    {Plugin::LifecycleState::INITIALIZING, _TXT("initializing")},
    {Plugin::LifecycleState::INACTIVE, _TXT("inactive")},
    {Plugin::LifecycleState::UNLOADING, _TXT("unloading")},
    {Plugin::LifecycleState::FOREGROUND, _TXT("foreground")},
    {Plugin::LifecycleState::BACKGROUND, _TXT("background")},
    {Plugin::LifecycleState::SUSPENDED, _TXT("suspended")},
    ENUM_CONVERSION_END(Plugin::LifecycleState)

namespace Plugin {

        const std::string ToString(LifecycleState state)
        {
            WPEFramework::Core::EnumerateType<LifecycleState> type(state);
            return type.Data();
        }

    AppBasicInfo JAppBasicInfo::GetAppInfo() const {
            AppBasicInfo info;
            info.id = id.Value();
            if (catalog.IsSet()) {
                info.catalog = catalog.Value();
            }
            if (url.IsSet()) {
                info.url = url.Value();
            }
            if (title.IsSet()) {
                info.title = title.Value();
            }
            return info;
    }

    bool AppRegistry::GetAppIntent(const std::string& appId, string& intent_str) {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        // Loop all entries to find app Id which matches the parameter 
        for (const auto& pair : mSessionMap) {
            if (pair.second.MatchAppId(appId) ) {
                pair.second.IntentToString(intent_str);
                return true;
            }
        }
        return false;
    }

    AppSession AppRegistry::Get(const std::string& key) {
        AppSession session;
        std::lock_guard<std::mutex> lock(mSessionMutex);
        auto it = mSessionMap.find(key);
        if (it != mSessionMap.end())
        {
            session = it->second;
        }
        return session;
    }

    bool JAppSessionResponse::Validate(string& error) const {
            if (!appId.IsSet()) {
                error = "appId is not set";
                return false;
            }
            if (!sessionId.IsSet()) {
                error = "sessionId is not set";
                return false;
            }
            if (!loadedSessionId.IsSet()) {
                error = "loadedSessionId is not set";
                return false;
            }
            if (!transitionPending.IsSet()) {
                error = "transitionPending is not set";
                return false;
            }
            return true;
    }

    bool SessionInfoRegistry::Get(const std::string& key, SessionInfo& session) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                auto it = mSessionMap.find(key);
                if (it != mSessionMap.end()) {
                    session = it->second;
                    return true;
                }
                return false;
    }

    bool LifecycleInfoRegistry::Get(const std::string& key, LifecycleInfo& session) {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        auto it = mLifecycleMap.find(key);
        if (it != mLifecycleMap.end()) {
            session = it->second;
            return true;
        }
        return false;
    }

    void LifecycleInfoRegistry::AddInitial(const std::string& key) {
        LifecycleInfo info;
        info.state = LifecycleState::INITIALIZING;
        info.previous = LifecycleState::INITIALIZING;
        info.source = LifecycleSource::none;
        Add(key, info);
    }

    bool LifecycleInfoRegistry::UpdateLifecycle(const std::string& key, const LifecycleState state, LifecycleState& previous) {
        std::lock_guard<std::mutex> lock(mSessionMutex);
        auto it = mLifecycleMap.find(key);
        if (it != mLifecycleMap.end()) {
            auto lifecycle = it->second;
            previous = lifecycle.state;
            lifecycle.previous = lifecycle.state;
            lifecycle.state = state;
            it->second = lifecycle;
            return true;
        }
        return false;
    }

    bool LifecycleChange::Validate(string& error) const {
        if (!state.IsSet()) {
            error = "state is not set";
            return false;
        }
        if (!previous.IsSet()) {
            error = "previous is not set";
            return false;
        }
        return true;
    }

    LifecycleInfo LifecycleChange::getLifecycleInfo() {
        LifecycleInfo info;
        info.state = state.Value();
        info.previous = previous.Value();
        info.source = LifecycleSource::none;
        return info;
    }
}

}