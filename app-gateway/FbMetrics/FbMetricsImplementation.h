/*
 * Copyright 2023 Comcast Cable Communications Management, LLC
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
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "Module.h"
#include <interfaces/IFbMetrics.h>
#include <interfaces/IConfiguration.h>

#include <map>
#include <mutex>
#include <unordered_map>

namespace WPEFramework {
namespace Plugin {
    class FbMetricsImplementation : public Exchange::IFbMetrics, public Exchange::IConfiguration {
    private:
        FbMetricsImplementation(const FbMetricsImplementation&) = delete;
        FbMetricsImplementation& operator=(const FbMetricsImplementation&) = delete;

    public:
        FbMetricsImplementation();
        ~FbMetricsImplementation();

        BEGIN_INTERFACE_MAP(FbMetricsImplementation)
        INTERFACE_ENTRY(Exchange::IFbMetrics)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        END_INTERFACE_MAP

        // IFbMetrics interface
        Core::hresult Action(const Exchange::IFbMetrics::Context &context,
                             const string &category,
                             const string &type,
                             const string &parameters) override;

        Core::hresult AppInfo(const Exchange::IFbMetrics::Context &context,
                              const string &build) override;

        Core::hresult Error(const Exchange::IFbMetrics::Context &context,
                            const string &type,
                            const string &code,
                            const string &description,
                            const bool visible,
                            const string &parameters) override;

        Core::hresult MediaEnded(const Exchange::IFbMetrics::Context &context,
                                 const string &entityId) override;

        Core::hresult MediaLoadStart(const Exchange::IFbMetrics::Context &context,
                                     const string &entityId) override;

        Core::hresult MediaPause(const Exchange::IFbMetrics::Context &context,
                                 const string &entityId) override;

        Core::hresult MediaPlay(const Exchange::IFbMetrics::Context &context,
                                const string &entityId) override;

        Core::hresult MediaPlaying(const Exchange::IFbMetrics::Context &context,
                                   const string &entityId) override;

        Core::hresult MediaProgress(const Exchange::IFbMetrics::Context &context,
                                    const string &entityId,
                                    const string &progress) override;

        Core::hresult MediaRateChange(const Exchange::IFbMetrics::Context &context,
                                      const string &entityId,
                                      const double rate) override;

        Core::hresult MediaRenditionChange(const Exchange::IFbMetrics::Context &context,
                                           const string &entityId,
                                           const uint32_t bitrate,
                                           const uint32_t width,
                                           const uint32_t height,
                                           const string &profile) override;

        Core::hresult MediaSeeked(const Exchange::IFbMetrics::Context &context,
                                  const string &entityId,
                                  const string &position) override;

        Core::hresult MediaSeeking(const Exchange::IFbMetrics::Context &context,
                                   const string &entityId,
                                   const string &target) override;

        Core::hresult MediaWaiting(const Exchange::IFbMetrics::Context &context,
                                   const string &entityId) override;

        Core::hresult Page(const Exchange::IFbMetrics::Context &context,
                           const string &pageId) override;

        Core::hresult StartContent(const Exchange::IFbMetrics::Context &context,
                                   const string &entityId) override;

        Core::hresult StopContent(const Exchange::IFbMetrics::Context &context,
                                  const string &entityId) override;

        Core::hresult SignIn(const Context& context,
                             const string&  entitlements) override;

        Core::hresult SignOut(const Context& context) override;

        Core::hresult SetAppUserSessionId(const string& id) override;

        Core::hresult SetLifeCycle(const Context& context, const string&  newState, const string&  previousState) override;

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* shell);

    private:

        class AppVersionRegistry
        {
        public:
            void Add(const std::string &appId, const std::string &appVersion)
            {
                std::lock_guard<std::mutex> lock(mAppVersionMutex);
                mAppVersionMap[appId] = appVersion;
            }

            void Remove(const std::string &appId)
            {
                std::lock_guard<std::mutex> lock(mAppVersionMutex);
                mAppVersionMap.erase(appId);
            }

            bool Get(const std::string &appId, std::string &appVersion)
            {
                std::lock_guard<std::mutex> lock(mAppVersionMutex);
                auto it = mAppVersionMap.find(appId);
                if (it != mAppVersionMap.end())
                {
                    appVersion = it->second;
                    return true;
                }
                return false;
            }

        private:
            std::unordered_map<std::string, std::string> mAppVersionMap;
            std::mutex mAppVersionMutex;
        };


        class AppUserSessionIdRegistry
        {
        public:
            void Set(const std::string& appSessionId)
            {
                std::lock_guard<std::mutex> lock(mAppSessionIdMutex);
                mAppSessionId = appSessionId;
            }

            void Clear()
            {
                std::lock_guard<std::mutex> lock(mAppSessionIdMutex);
                mAppSessionId.clear();
            }

            std::string Get()
            {
                std::lock_guard<std::mutex> lock(mAppSessionIdMutex);
                return mAppSessionId;
            }

        private:
            std::string mAppSessionId;
            std::mutex mAppSessionIdMutex;
        };

        std::string GetAppSessionId(const string &appId);
        Core::hresult SendAnalyticsEvent(const string &eventName, const string &payload, const string &appId);
        void UpdateWithAppContext(const Exchange::IFbMetrics::Context &context,
                                 JsonObject &eventPayload);

        PluginHost::IShell* mShell;
        AppVersionRegistry mAppVersionCache;
        AppUserSessionIdRegistry mAppUserSessionIdRegistry;
    };
}
}
