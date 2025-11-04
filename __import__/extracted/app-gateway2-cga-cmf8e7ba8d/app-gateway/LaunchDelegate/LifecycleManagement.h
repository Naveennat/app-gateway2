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

#pragma once

#include "Module.h"
#include <mutex>
#include <map>

namespace WPEFramework {
namespace Plugin {

    struct AppBasicInfo {
        string id;
        string catalog;
        string url;
        string title;
    };

    class JAppBasicInfo: public Core::JSON::Container {
        public:
        Core::JSON::String id; // Application Id
        Core::JSON::String catalog; // Application catalog info
        Core::JSON::String url; // Application url
        Core::JSON::String title; // Application title

        JAppBasicInfo() : Core::JSON::Container()
        {
            Add(_T("id"), &id);
            Add(_T("catalog"), &catalog);
            Add(_T("url"), &url);
            Add(_T("title"), &title);
        }
        
        bool Validate(string& error) const {
            if (!id.IsSet()) {
                error = "App id is not set";
                return false;
            }
            return true;
        }

        AppBasicInfo GetAppInfo() const;
    };

    struct AppLaunchInfo {
        JsonValue intent; // Raw JSON Intent to launch the application
        bool inactive {false}; // Whether to launch the app in suspended mode
    };

    struct AppSession {
        AppBasicInfo app;
        AppLaunchInfo launch;

        string GetAppId() {
            return app.id;
        }

        JsonValue GetIntent() {
            return launch.intent;
        }

        string GetCatalog() {
            return app.catalog;
        }

        bool IsInactive() {
            return launch.inactive;
        }

        bool MatchAppId(const std::string& appId) const {
            return app.id == appId;
        }

        void IntentToString(string& str) const {
            if (launch.intent.IsSet()) {
                str = launch.intent.String();
            } else {
                str = "{}";
            }
        }
    };

    class JAppSession: public Core::JSON::Container {
        public:
            JAppBasicInfo app; // Basic application information
            JsonObject launch; // Application launch information

        JAppSession() : Core::JSON::Container()
        {
            Add(_T("app"), &app);
            Add(_T("launch"), &launch);
        }
        
        bool Validate(string& error) const {
            return app.Validate(error);
        }

        AppLaunchInfo GetLaunchInfo() const {
            AppLaunchInfo info;
            info.intent = launch["intent"];
            info.inactive = launch["inactive"].Boolean();
            return info;
        }

        AppSession GetSession() const {
            AppSession session;
            session.app = app.GetAppInfo();
            session.launch = GetLaunchInfo();
            return session;
        }
    };

    class AppRegistry{
        public:
            void Add(const std::string& key, const AppSession &session) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                mSessionMap[key] = session;
            }

            void Remove(const std::string& key) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                mSessionMap.erase(key);
            }

            // If session is available update AppId and return true. appId is available in session.app.id
            // else return false
            bool GetAppId(const std::string& key, std::string& appId) {
                AppSession session = Get(key);
                appId = session.GetAppId();
                return !appId.empty();
            }

            bool GetAppIntent(const std::string& appId, string& intent_str);

        private:

            AppSession Get(const std::string& key);
            
            std::unordered_map<std::string, AppSession> mSessionMap;
            std::mutex mSessionMutex;
    };

    class JAppSessionRequest: public Core::JSON::Container {
        public:
            JAppSession jSession; // Application session information

        JAppSessionRequest() : Core::JSON::Container()
        {
            Add(_T("session"), &jSession);
        }
        
        bool Validate(string& error) const {
            return jSession.Validate(error);
        }

        AppSession GetSession() const {
            return jSession.GetSession();
        }

    };

    struct SessionInfo{
        string appId;
        string sessionId;
        string loadedSessionId;
        string activeSessionId;
    };

    class JAppSessionResponse: public Core::JSON::Container {
        public:
            Core::JSON::String appId; // Application Id
            Core::JSON::String sessionId; // Application session Id
            Core::JSON::String loadedSessionId; // Application loaded session Id
            Core::JSON::String activeSessionId; // Application active session Id
            Core::JSON::Boolean transitionPending; // Whether the application is in transition state


        JAppSessionResponse() : Core::JSON::Container()
        {
            Add(_T("appId"), &appId);
            Add(_T("sessionId"), &sessionId);
            Add(_T("loadedSessionId"), &loadedSessionId);
            Add(_T("activeSessionId"), &activeSessionId);
            Add(_T("transitionPending"), &transitionPending);
        }
        
        bool Validate(string& error) const;

        SessionInfo getSessionInfo() {
            SessionInfo info;
            info.appId = appId.Value();
            info.sessionId = sessionId.Value();
            info.loadedSessionId = loadedSessionId.Value();
            info.activeSessionId = activeSessionId.Value();
            return info;
        }
    };

    class SessionInfoRegistry{
        public:
            void Add(const std::string& key, const SessionInfo &session) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                mSessionMap[key] = session;
            }

            void Remove(const std::string& key) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                mSessionMap.erase(key);
            }

            bool Get(const std::string& key, SessionInfo& session);


        private:
            std::unordered_map<std::string, SessionInfo> mSessionMap;
            std::mutex mSessionMutex;
    };
   
     enum class LifecycleState: uint8_t {
        INITIALIZING = 0,
        INACTIVE,
        UNLOADING,
        FOREGROUND,
        BACKGROUND,
        SUSPENDED
    };

    const std::string ToString(LifecycleState state);

    enum LifecycleSource {
        voice,
        remote,
        none
    };

    struct LifecycleInfo{
        LifecycleState state;
        LifecycleState previous;
        LifecycleSource source;
    };

    class LifecycleInfoRegistry{
        public:
            void Add(const std::string& key, const LifecycleInfo& session) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                mLifecycleMap[key] = session;
            }

            void Remove(const std::string& key) {
                std::lock_guard<std::mutex> lock(mSessionMutex);
                mLifecycleMap.erase(key);
            }

            bool Get(const std::string& key, LifecycleInfo& session);

            void AddInitial(const std::string& key);

            bool UpdateLifecycle(const std::string& key, const LifecycleState state, LifecycleState& previous);

        private:
            std::unordered_map<std::string, LifecycleInfo> mLifecycleMap;
            std::mutex mSessionMutex;
    };

    class LifecycleChange: public Core::JSON::Container {
        public:
            Core::JSON::EnumType<LifecycleState> state;
            Core::JSON::EnumType<LifecycleState> previous;
        
        LifecycleChange() : Core::JSON::Container() {
            Add(_T("state"), &state);
            Add(_T("previous"), &previous);
        }

        bool Validate(string& error) const;
        
        LifecycleInfo getLifecycleInfo();
    };

    class JSetSession: public Core::JSON::Container {
        public:
            Core::JSON::EnumType<LifecycleState> state;
            Core::JSON::String appId;

            JSetSession() : Core::JSON::Container() {
                Add(_T("state"), &state);
                Add(_T("appId"), &appId);
            }

            string GetAppId() const {
                return appId.Value();
            }

            LifecycleState GetState() const {
                return state.Value();
            }

    };

    class AppSessionIdRegistry
    {
    public:
        void Add(const std::string &appId, const std::string &appSessionId)
        {
            std::lock_guard<std::mutex> lock(mAppSessionIdMutex);
            mAppSessionIdMap[appId] = appSessionId;
        }

        void Remove(const std::string &appId)
        {
            std::lock_guard<std::mutex> lock(mAppSessionIdMutex);
            mAppSessionIdMap.erase(appId);
        }

        bool Get(const std::string &appId, std::string &sessionId)
        {
            std::lock_guard<std::mutex> lock(mAppSessionIdMutex);
            auto it = mAppSessionIdMap.find(appId);
            if (it != mAppSessionIdMap.end())
            {
                sessionId = it->second;
                return true;
            }
            return false;
        }

    private:
        std::unordered_map<std::string, std::string> mAppSessionIdMap;
        std::mutex mAppSessionIdMutex;
    };

    class ContentPartnerIdRegistry
    {
    public:
        void Add(const std::string &appId, const std::string &contentPartnerId)
        {
            std::lock_guard<std::mutex> lock(mContentPartnerIdMutex);
            mContentPartnerIdMap[appId] = contentPartnerId;
        }

        void Remove(const std::string &appId)
        {
            std::lock_guard<std::mutex> lock(mContentPartnerIdMutex);
            mContentPartnerIdMap.erase(appId);
        }

        bool Get(const std::string &appId, std::string &contentPartnerId)
        {
            std::lock_guard<std::mutex> lock(mContentPartnerIdMutex);
            auto it = mContentPartnerIdMap.find(appId);
            if (it != mContentPartnerIdMap.end())
            {
                contentPartnerId = it->second;
                return true;
            }
            return false;
        }

    private:
        std::unordered_map<std::string, std::string> mContentPartnerIdMap;
        std::mutex mContentPartnerIdMutex;
    };
}
}