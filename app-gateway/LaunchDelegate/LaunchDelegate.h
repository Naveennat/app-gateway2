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
#include <interfaces/ILaunchDelegate.h>
#include <interfaces/IAppNotifications.h>
#include "core/core.h"
#include "LifecycleManagement.h"
#include "UtilsUUID.h"
#include "WsManager.h"
#include "UtilsLogging.h"
#include "UtilsFirebolt.h"
#include <interfaces/IAppGateway.h>
#include "ProviderDelegate.h"
#include "UtilsCallsign.h"


#define LAUNCHDELEGATE_SOCKET_ADDRESS "0.0.0.0:3474"

//#define LAUNCHDELEGATE_SOCKET_ADDRESS "127.0.0.1:3475"


namespace WPEFramework {
namespace Plugin {
    using Context = Exchange::GatewayContext;
    static const std::map<std::string, LifecycleState> StringToLifecycleState = {
		{"initializing", LifecycleState::INITIALIZING},
		{"inactive", LifecycleState::INACTIVE},
		{"unloading", LifecycleState::UNLOADING},
		{"foreground", LifecycleState::FOREGROUND},
		{"background", LifecycleState::BACKGROUND},
		{"suspended", LifecycleState::SUSPENDED}
	};


    class LaunchDelegate : public PluginHost::IPlugin, public PluginHost::JSONRPC, public Exchange::ILaunchDelegate, public Exchange::IAppNotificationHandler,
    public Exchange::IAppGatewayResponder, public Exchange::IAppGatewayAuthenticator, public Exchange::IAppGatewayRequestHandler {
    private:
        LaunchDelegate(const LaunchDelegate&) = delete;
        LaunchDelegate& operator=(const LaunchDelegate&) = delete;
   
    public:
        LaunchDelegate();
        virtual ~LaunchDelegate();
        // IPlugin interface implementation
        virtual const string Initialize(PluginHost::IShell* service) override;
        virtual void Deinitialize(PluginHost::IShell* service) override;
        virtual string Information() const override;
       
         private:
            void Deactivated(RPC::IRemoteConnection* connection);
	public:
        BEGIN_INTERFACE_MAP(LaunchDelegate)
        INTERFACE_ENTRY(PluginHost::IPlugin)
        INTERFACE_ENTRY(PluginHost::IDispatcher)
        INTERFACE_ENTRY(Exchange::ILaunchDelegate)
        INTERFACE_ENTRY(Exchange::IAppNotificationHandler)
        INTERFACE_ENTRY(Exchange::IAppGatewayResponder)
        INTERFACE_ENTRY(Exchange::IAppGatewayAuthenticator)
        INTERFACE_ENTRY(Exchange::IAppGatewayRequestHandler)
        END_INTERFACE_MAP
    
    public:

        Core::hresult Ready(const Context& context, string& result );

        Core::hresult Close(const Context& context ,
                const string& payload , string& result  /* @out @opaque */);	

        Core::hresult Finished( const Context& context, string& result  /* @out @opaque */ );


        Core::hresult Authenticate( const string& sessionId , std::string& appId) override;

        Core::hresult CheckPermissionGroup(const string& appId /* @in */,
                                                       const string& permissionGroup /* @in */,
                                                       bool& allowed /* @out */) override;

        Core::hresult GetIntent( const string& appId , std::string& intent );

        Core::hresult Respond(const Context& context /* @in */,
                                        const string& payload /* @in @opaque */) override;
        
        Core::hresult Emit(const Context& context /* @in */, 
                const string& method /* @in */, const string& payload /* @in @opaque */) override;

        Core::hresult Request(const uint32_t connectionId /* @in */, 
                const uint32_t id /* @in */, const string& method /* @in */, const string& params /* @in @opaque */) override;

        Core::hresult GetGatewayConnectionContext(const uint32_t connectionId /* @in */,
                const string& contextKey /* @in */, 
                 string& contextValue /* @out */) override;

        Core::hresult Register(Exchange::IAppGatewayResponder::INotification *notification) override;
        Core::hresult Unregister(Exchange::IAppGatewayResponder::INotification *notification) override;

        void OnConnectionStatusChanged(const string& appId, const uint32_t& connectionId, const bool& connected);


        Core::hresult HandleAppEventNotifier(const string& event, const bool& listen, bool& status /* @out */) override;

        Core::hresult State( const Context& context /* @in */ ,string& state  /* @out */);

        Core::hresult Initialization(const Context& context, string& response);

        Core::hresult GetSessionId(const string& appId, string& sessionId) override;

        Core::hresult GetContentPartnerId(const string& appId, string& contentPartnerId) override;

        Core::hresult HandleAppGatewayRequest(const Context &context /* @in */,
                                          const string& method /* @in */,
                                          const string &payload /* @in @opaque */,
                                          string& result /*@out @opaque */) override;

        
        
        bool getAppIdFromSessId( const std::string& sessionID,  std::string& appId ) ;//Session

        // Create a method which accepts a string and reference to LifecycleState
	    // converts string to LifecycleState and returns a boolean if there is no match
	    // check MUST be case in sensitve
	    bool GetLifecycleStateFromString(const string& state, LifecycleState& state_value) {
		    auto it = StringToLifecycleState.find(state);
		    if (it != StringToLifecycleState.end()) {
                state_value = it->second;
                return true;
            }
		    return false;
	    }

        // Get the shell service
        PluginHost::IShell* GetService() const
        {
            return mService;
        }
    private:
        class EXTERNAL ResolveJob : public Core::IDispatch
        {
            protected:
                ResolveJob(LaunchDelegate *parent, 
                const uint32_t connectionId,
                const uint32_t requestId,
                const string& method,
                const string& params)
                    : mParent(*parent), mMethod(method), mParams(params), mRequestId(requestId), mConnectionId(connectionId)
                {

                }
            public:
                ResolveJob() = delete;
                ResolveJob(const ResolveJob &) = delete;
                ResolveJob &operator=(const ResolveJob &) = delete;
                ~ResolveJob()
                {
                }
            
            public:
                static Core::ProxyType<Core::IDispatch> Create(LaunchDelegate *parent,
                const uint32_t connectionId, const uint32_t requestId, const std::string& method, const std::string& params)
                {
                    return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<ResolveJob>::Create(parent, connectionId, requestId, method, params)));
                }
                virtual void Dispatch()
                {
                    mParent.InternalResolve(mConnectionId, mRequestId, mMethod, mParams);
                }
            private:
                LaunchDelegate &mParent;
                const std::string mMethod;
                const std::string mParams;
                const uint32_t mRequestId;
                const uint32_t mConnectionId;
        };

        class EXTERNAL DispatchLifecycleJob : public Core::IDispatch 
        {
            protected:
                DispatchLifecycleJob(LaunchDelegate *parent, const std::string &appId, const LifecycleState newState, const LifecycleState oldState)
                    : mParent(*parent), mAppId(appId), mNewState(newState), mOldState(oldState)
                {
                }

            public:
                static Core::ProxyType<Core::IDispatch> Create(LaunchDelegate *parent, const std::string appId, const LifecycleState newState, const LifecycleState oldState)
                {
                    return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<DispatchLifecycleJob>::Create(parent, appId, newState, oldState)));
                }

                virtual void Dispatch()
                {
                    mParent.DispatchLifecycleChange(mAppId, mNewState, mOldState);
                }

            private:
                LaunchDelegate &mParent;
                const std::string mAppId;
                const LifecycleState mNewState;
                const LifecycleState mOldState;
        };

        class EXTERNAL RespondJob : public Core::IDispatch
        {
        protected:
            RespondJob(LaunchDelegate *parent, 
            const uint32_t connectionId,
            const uint32_t requestId,
            const std::string& payload
            )
                : mParent(*parent), mPayload(payload), mRequestId(requestId), mConnectionId(connectionId)
            {
            }

        public:
            RespondJob() = delete;
            RespondJob(const RespondJob &) = delete;
            RespondJob &operator=(const RespondJob &) = delete;
            ~RespondJob()
            {
            }

        public:
            static Core::ProxyType<Core::IDispatch> Create(LaunchDelegate *parent,
                const uint32_t connectionId, const uint32_t requestId, const std::string& payload)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<RespondJob>::Create(parent, connectionId, requestId, payload)));
            }
            virtual void Dispatch()
            {
                mParent.mWsManager.SendMessageToConnection(mConnectionId, mPayload, mRequestId);
            }

        private:
            LaunchDelegate &mParent;
            const std::string mPayload;
            const uint32_t mRequestId;
            const uint32_t mConnectionId;
        };

        class EXTERNAL EmitJob : public Core::IDispatch
        {
        protected:
            EmitJob(LaunchDelegate *parent, 
            const uint32_t connectionId,
            const std::string& designator,
            const std::string& payload
            )
                : mParent(*parent), mPayload(payload), mDesignator(designator), mConnectionId(connectionId)
            {
            }

        public:
            EmitJob() = delete;
            EmitJob(const EmitJob &) = delete;
            EmitJob &operator=(const EmitJob &) = delete;
            ~EmitJob()
            {
            }

        public:
            static Core::ProxyType<Core::IDispatch> Create(LaunchDelegate *parent,
                const uint32_t connectionId, const std::string& designator, const std::string& payload)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<EmitJob>::Create(parent, connectionId, designator, payload)));
            }
            virtual void Dispatch()
            {
                mParent.mWsManager.DispatchNotificationToConnection(mConnectionId, mPayload, mDesignator);
            }

        private:
            LaunchDelegate &mParent;
            const std::string mPayload;
            const std::string mDesignator;
            const uint32_t mConnectionId;
        };

        class EXTERNAL RequestJob : public Core::IDispatch
        {
        protected:
            RequestJob(LaunchDelegate *parent, 
            const uint32_t connectionId,
            const uint32_t requestId,
            const std::string& designator,
            const std::string& payload
            )
                : mParent(*parent), mPayload(payload), mDesignator(designator), mConnectionId(connectionId), mRequestId(requestId)
            {
            }

        public:
            RequestJob() = delete;
            RequestJob(const RequestJob &) = delete;
            RequestJob &operator=(const RequestJob &) = delete;
            ~RequestJob()
            {
            }

        public:
            static Core::ProxyType<Core::IDispatch> Create(LaunchDelegate *parent,
                const uint32_t connectionId, const uint32_t mRequestId, const std::string& designator, const std::string& payload)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<RequestJob>::Create(parent, connectionId, mRequestId, designator, payload)));
            }
            virtual void Dispatch()
            {
                mParent.mWsManager.SendRequestToConnection(mConnectionId, mDesignator, mRequestId, mPayload);
            }

        private:
            LaunchDelegate &mParent;
            const std::string mPayload;
            const std::string mDesignator;
            const uint32_t mConnectionId;
            const uint32_t mRequestId;
        };

        class EXTERNAL ConnectionStatusNotificationJob : public Core::IDispatch
        {
        protected:
            ConnectionStatusNotificationJob(LaunchDelegate *parent,
            const uint32_t connectionId,
            const std::string& appId,
            const bool connected
            )
                : mParent(*parent), mConnectionId(connectionId), mAppId(appId), mConnected(connected)
            {
            }

        public:
            ConnectionStatusNotificationJob() = delete;
            ConnectionStatusNotificationJob(const ConnectionStatusNotificationJob &) = delete;
            ConnectionStatusNotificationJob &operator=(const ConnectionStatusNotificationJob &) = delete;
            ~ConnectionStatusNotificationJob()
            {
            }

        public:
            static Core::ProxyType<Core::IDispatch> Create(LaunchDelegate *parent,
                const uint32_t connectionId, const std::string& appId, const bool connected)
            {
                return (Core::ProxyType<Core::IDispatch>(Core::ProxyType<ConnectionStatusNotificationJob>::Create(parent, connectionId, appId, connected)));
            }
            virtual void Dispatch()
            {
                mParent.OnConnectionStatusChanged(mAppId, mConnectionId, mConnected);
            }

        private:
            LaunchDelegate &mParent;
            const uint32_t mConnectionId;
            const std::string mAppId;
            const bool mConnected;
        };


        uint32_t mConnectionId;
        PluginHost::IShell* mService;
        void RegisterAll();
        void UnregisterAll();
	    void updateAppLifeCycleState(const std::string& appId, const LifecycleState& newState, LifecycleState& oldState);
        void DispatchLifecycleChange(const std::string& appId, const LifecycleState& newState, const LifecycleState& oldState);
        WebSocketConnectionManager mWsManager;
        // Store by Session Id key easier to retrieve for Authentication
        AppRegistry mAppRegistry;
        // Store by App Id Key needed for future usecases for metrics
        SessionInfoRegistry mSessionRegistry;
        // Store by App Id for easy retrieval
        LifecycleInfoRegistry mLifecycleRegistry;
        // Store by App Id
        AppSessionIdRegistry mAppSessionIdRegistry;
        // Store by App Id
        ContentPartnerIdRegistry mContentPartnerIdRegistry;

        // Provider Registry
        ProviderRegistry mProviderRegistry;

        // Handle ProviderRegistrations
        uint32_t HandleProviderRegistration(const Core::JSONRPC::Context& channel, const string& event, const bool& listen);

        void DispatchProvider(const string& event, const JsonObject& data);

        void InternalResolve(const uint32_t connectionId, const uint32_t requestId, const string& method, const string& params);


   public:
	std::string GeneratedSessionId()
	{
		return UtilsUUID::GenerateUUID();
	}
	// List of LifecycleManagement JSON RPC methods available for the websocket.
    uint32_t Session(const Core::JSONRPC::Context& channel, const JAppSessionRequest& request);
    uint32_t SetSession(const Core::JSONRPC::Context& channel, const JSetSession& request);
    uint32_t OnRequestReady(const Core::JSONRPC::Context& channel, const JListenRequest& request );
    uint32_t OnRequestClose(const Core::JSONRPC::Context& channel, const JListenRequest& request );
    uint32_t OnRequestFinished(const Core::JSONRPC::Context& channel, const JListenRequest& request );
    uint32_t OnRequestLaunch(const Core::JSONRPC::Context& channel, const JListenRequest& request );
    uint32_t OnSessionTransitionCompleted(const Core::JSONRPC::Context& channel, const JListenRequest& request );
    uint32_t OnSessionTransitionCanceled(const Core::JSONRPC::Context& channel, const JListenRequest& request );
    uint32_t AddContext(const Core::JSONRPC::Context& channel, const JsonObject& params);
    void initializeWebsocketServer();
    void initializeDelegates();
    Exchange::IAppGatewayResolver *mAppGateway;
    Exchange::IAppNotifications *mAppNotifications;
    Exchange::IAppGatewayResponder *mAppGatewayResponder;
    mutable Core::CriticalSection mConnectionStatusImplLock;
    std::list<Exchange::IAppGatewayResponder::INotification*> mConnectionStatusNotification;
    bool SetupAppGatewayResponder();
    bool SetupAppNotifications();
    ProviderDelegate mProviderDelegate;


    class AppGatewayConnectionChanged : public Exchange::IAppGatewayResponder::INotification
    {
    public:
        AppGatewayConnectionChanged(LaunchDelegate &parent) : mParent(parent) {}
        ~AppGatewayConnectionChanged() {}

        void OnAppConnectionChanged(const string& appId, const uint32_t connectionId, const bool& connected)
        {
            LOGDBG("OnAppConnectionChanged: appId=%s, connectionId=%d, connected=%d", appId.c_str(), connectionId, connected);
            if (!connected) {
                mParent.mProviderDelegate.Cleanup(connectionId, APP_GATEWAY_CALLSIGN);
            }
        }

        BEGIN_INTERFACE_MAP(AppGatewayConnectionChanged)
        INTERFACE_ENTRY(Exchange::IAppGatewayResponder::INotification)
        END_INTERFACE_MAP

    private:
        LaunchDelegate &mParent;
    };

    Core::Sink<AppGatewayConnectionChanged> mAppGatewayConnectionChangedSink;
};

} // namespace Plugin
} // namespace WPEFramework


