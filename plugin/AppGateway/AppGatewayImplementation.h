/*
* If not stated otherwise in this file or this component's LICENSE file the
* following copyright and licenses apply:
*
* Copyright 2025 RDK Management
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
#include "Resolver.h"
#include <interfaces/IAppGateway.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IAppNotifications.h>
#include "ContextUtils.h"
#include "ContextConversionHelpers.h"
#include <com/com.h>
#include <core/core.h>
#include <atomic>
#include <map>


namespace WPEFramework {
namespace Plugin {
    using Context = Exchange::GatewayContext;
    class AppGatewayImplementation : public Exchange::IAppGatewayResolver, public Exchange::IConfiguration
    {

    public:
        AppGatewayImplementation();
        ~AppGatewayImplementation() override;

        // We do not allow this plugin to be copied !!
        AppGatewayImplementation(const AppGatewayImplementation&) = delete;
        AppGatewayImplementation& operator=(const AppGatewayImplementation&) = delete;

        BEGIN_INTERFACE_MAP(AppGatewayImplementation)
        INTERFACE_ENTRY(Exchange::IConfiguration)
        INTERFACE_ENTRY(Exchange::IAppGatewayResolver)
        END_INTERFACE_MAP

    public:
        Core::hresult Configure(Exchange::IAppGatewayResolver::IStringIterator *const &paths) override;
        Core::hresult Resolve(const Context& context, const string& origin ,const string& method, const string& params, string& result) override;

        // IConfiguration interface
        uint32_t Configure(PluginHost::IShell* service) override;

    private:
        // Tracks whether we are shutting down. Any async dispatch should early-return once true.
        std::atomic<bool> mIsShuttingDown{ false };

        // Tracks number of queued/in-flight async jobs that may call back into this instance.
        std::atomic<uint32_t> mInFlightJobs{ 0 };

        // Small helper to ensure we decrement the job counter on all paths.
        class JobTracker final {
        public:
            JobTracker() = delete;
            explicit JobTracker(std::atomic<uint32_t>& counter)
                : mCounter(counter)
                , mArmed(true)
            {
                mCounter.fetch_add(1, std::memory_order_acq_rel);
            }
            JobTracker(const JobTracker&) = delete;
            JobTracker& operator=(const JobTracker&) = delete;

            ~JobTracker()
            {
                if (mArmed) {
                    mCounter.fetch_sub(1, std::memory_order_acq_rel);
                }
            }

            void Disarm() { mArmed = false; }

        private:
            std::atomic<uint32_t>& mCounter;
            bool mArmed;
        };

        class EXTERNAL RespondJob : public Core::IDispatch
        {
        public:
            RespondJob() = delete;
            RespondJob(const RespondJob&) = delete;
            RespondJob& operator=(const RespondJob&) = delete;

            RespondJob(AppGatewayImplementation* parent,
                       const Context& context,
                       const std::string& payload,
                       const std::string& destination)
                : mParent(parent)
                , mPayload(payload)
                , mContext(context)
                , mDestination(destination)
                , mTracker((parent != nullptr) ? parent->mInFlightJobs : mDummyCounter)
            {
                // IMPORTANT:
                // Do NOT AddRef/Release mParent here.
                // In L0/in-proc tests, AppGatewayImplementation can be stack-allocated, and
                // COM-style refcounting would cause delete-this on stack memory (UB/SIGSEGV).
                //
                // Instead:
                // - Count in-flight jobs (best-effort drain during teardown)
                // - Gate execution on mIsShuttingDown and on mService/responder availability.
            }

            ~RespondJob() override = default;

        public:
            static Core::ProxyType<Core::IDispatch> Create(AppGatewayImplementation* parent,
                                                          const Context& context,
                                                          const std::string& payload,
                                                          const std::string& origin)
            {
                return Core::ProxyType<Core::IDispatch>(
                    Core::ProxyType<RespondJob>::Create(parent, context, payload, origin));
            }

            void Dispatch() override
            {
                if (mParent == nullptr) {
                    return;
                }

                // If teardown has started, do not call back into the object.
                if (mParent->mIsShuttingDown.load(std::memory_order_acquire) == true) {
                    return;
                }

                if (ContextUtils::IsOriginGateway(mDestination)) {
                    mParent->ReturnMessageInSocket(mContext, mPayload);
                } else {
                    mParent->SendToLaunchDelegate(mContext, mPayload);
                }
            }

        private:
            AppGatewayImplementation* mParent;
            std::string mPayload;
            Context mContext;
            std::string mDestination;

            // If constructed with null parent, keep a dummy counter reference (never used).
            static std::atomic<uint32_t> mDummyCounter;
            JobTracker mTracker;
        };

        Core::hresult HandleEvent(const Context &context, const string &alias, const string &event, const string &origin,  const bool listen);
                
        void ReturnMessageInSocket(const Context& context, const string payload )
        {
            // This method can be called from an async worker thread. If the plugin is
            // shutting down, the service pointer may no longer be valid.
            if (mService == nullptr) {
                LOGERR("AppGateway service not available (shutdown in progress); dropping response");
                return;
            }

            if (mAppGatewayResponder == nullptr) {
                mAppGatewayResponder = mService->QueryInterface<Exchange::IAppGatewayResponder>();
            }

            if (mAppGatewayResponder == nullptr) {
                LOGERR("AppGateway Responder not available");
                return;
            }

            if (Core::ERROR_NONE != mAppGatewayResponder->Respond(context, payload)) {
                LOGERR("Failed to Respond in Gateway");
            }
        }

        PluginHost::IShell* mService;
        ResolverPtr mResolverPtr;
        Exchange::IAppNotifications *mAppNotifications; // Shared pointer to AppNotifications
        Exchange::IAppGatewayResponder *mAppGatewayResponder;
        Exchange::IAppGatewayResponder *mInternalGatewayResponder; // Shared pointer to InternalGatewayResponder
        Exchange::IAppGatewayAuthenticator *mAuthenticator; // Shared pointer to Authenticator
        uint32_t InitializeResolver();
        uint32_t InitializeWebsocket();
        uint32_t ProcessComRpcRequest(const Context &context, const string& alias, const string& method, const string& params, const string& origin, string &resolution);
        uint32_t PreProcessEvent(const Context &context, const string& alias, const string &method, const string& origin, const string& params, string &resolution);
        string UpdateContext(const Context &context, const string& method, const string& params, const string& origin, const bool& onlyAdditionalContext = false);
        Core::hresult InternalResolve(const Context &context, const string &method, const string &params, const string &origin, string& resolution);
        Core::hresult FetchResolvedData(const Context &context, const string &method, const string &params, const string &origin, string& resolution);
        Core::hresult InternalResolutionConfigure(std::vector<std::string>&& configPaths);
        bool SetupAppGatewayAuthenticator();
        void SendToLaunchDelegate(const Context& context, const string& payload);
        std::string ReadCountryFromConfigFile();
    };
} // namespace Plugin
} // namespace WPEFramework
