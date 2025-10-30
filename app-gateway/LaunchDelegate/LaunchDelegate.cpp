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

#include "LaunchDelegate.h"
#include <interfaces/ILaunchDelegate.h>
#include <interfaces/IFbPrivacy.h>
#include <interfaces/IConfiguration.h>
#include <interfaces/IFbMetrics.h>
#include "UtilsLogging.h"
#include "UtilsConnections.h"
#include "UtilsFirebolt.h"
#include "StringUtils.h"
#include "ContextUtils.h"
#include "ObjectUtils.h"

#define API_VERSION_NUMBER_MAJOR    LAUNCHDELEGATE_MAJOR_VERSION
#define API_VERSION_NUMBER_MINOR    LAUNCHDELEGATE_MINOR_VERSION
#define API_VERSION_NUMBER_PATCH    LAUNCHDELEGATE_PATCH_VERSION

#define LIFECYCLE_MANAGEMENT_MODULE "lifecyclemanagement"
#define METRICS_MANAGEMENT_MODULE "metricsmanagement"
#define INTEGRATED_PLAYER "integratedplayer"

namespace WPEFramework {
    
    namespace {
        static Plugin::Metadata<Plugin::LaunchDelegate> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {}
        );
    }

namespace Plugin {

    SERVICE_REGISTRATION(LaunchDelegate, 1,0,0 );

    LaunchDelegate::LaunchDelegate()
        : PluginHost::JSONRPC()
        , mService(nullptr)
		, mConnectionId(0)
        , mAppGateway(nullptr)
        , mAppNotifications(nullptr)
        , mAppGatewayResponder(nullptr),
        mAppGatewayConnectionChangedSink(*this)
    {
        LOGINFO("LaunchDelegate Constructor");
        RegisterAll();
    }

    LaunchDelegate::~LaunchDelegate()
    {
        if (nullptr != mService)
        {
            mService->Release();
            mService = nullptr;
        }


        if (nullptr != mAppGateway)
        {
            mAppGateway->Release();
            mAppGateway = nullptr;
        }

        if (nullptr != mAppNotifications)
        {
            mAppNotifications->Release();
            mAppNotifications = nullptr;
        }

        if (nullptr != mAppGatewayResponder)
        {
            mAppGatewayResponder->Release();
            mAppGatewayResponder = nullptr;
        }
    }

    /* virtual */ const string LaunchDelegate::Initialize(PluginHost::IShell* service)
    {
        LOGINFO("Init");
        ASSERT(service != nullptr);
        mService = service;
        mService->AddRef();
        initializeWebsocketServer();
        LOGINFO("LaunchDelegate::Initialize: PID=%u", getpid());            
        
        // On success return empty, to indicate there is no error text.
        return EMPTY_STRING;
    }

    /* virtual */ void LaunchDelegate::Deinitialize(PluginHost::IShell* service)
    {
        ASSERT(service == mService);

        mConnectionId = 0;
        mService->Release();
        mService = nullptr;
    }

    string LaunchDelegate::Information() const
        {
            // Return plugin information including WebSocket status
            string info = "LaunchDelegate with WebSocket support";
            return info;
        }

    void LaunchDelegate::Deactivated(RPC::IRemoteConnection* connection)
    {
        if (connection->Id() == mConnectionId) {

            ASSERT(mService != nullptr);

            Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(mService, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
        }
    }

    void LaunchDelegate::RegisterAll(){
        PluginHost::JSONRPC::Register<JAppSessionRequest>(_T("session"), &LaunchDelegate::Session, this);
        PluginHost::JSONRPC::Register<JListenRequest>(_T("onrequestready"), &LaunchDelegate::OnRequestReady, this);
        PluginHost::JSONRPC::Register<JListenRequest>(_T("onrequestclose"), &LaunchDelegate::OnRequestClose, this);
        PluginHost::JSONRPC::Register<JListenRequest>(_T("onrequestfinished"), &LaunchDelegate::OnRequestFinished, this);
        PluginHost::JSONRPC::Register<JListenRequest>(_T("onrequestlaunch"), &LaunchDelegate::OnRequestLaunch, this);
        PluginHost::JSONRPC::Register<JListenRequest>(_T("onsessiontransitioncompleted"), &LaunchDelegate::OnSessionTransitionCompleted, this);
        PluginHost::JSONRPC::Register<JListenRequest>(_T("onsessiontransitioncanceled"), &LaunchDelegate::OnSessionTransitionCanceled, this);
        PluginHost::JSONRPC::Register<JsonObject>(_T("addcontext"), &LaunchDelegate::AddContext, this );
        PluginHost::JSONRPC::Register<JSetSession>(_T("setstate"), &LaunchDelegate::SetSession, this );
    }
    
    void LaunchDelegate::UnregisterAll(){
        PluginHost::JSONRPC::Unregister(_T("session"));
        PluginHost::JSONRPC::Unregister(_T("onrequestready"));
        PluginHost::JSONRPC::Unregister(_T("onrequestclose"));
        PluginHost::JSONRPC::Unregister(_T("onrequestfinished"));
        PluginHost::JSONRPC::Unregister(_T("onrequestlaunch"));
        PluginHost::JSONRPC::Unregister(_T("onsessiontransitioncompleted"));
        PluginHost::JSONRPC::Unregister(_T("onsessiontransitioncanceled"));
        PluginHost::JSONRPC::Unregister(_T("addcontext"));
       PluginHost::JSONRPC::Unregister(_T("setstate"));
    }

    uint32_t LaunchDelegate::Session(const Core::JSONRPC::Context& channel, const JAppSessionRequest& request)
	{
            string errReason;
            // Validate the inputs
            if (!request.Validate(errReason)) {
                return Core::ERROR_BAD_REQUEST;
            }

            JAppSessionResponse response;

            AppSession session = request.GetSession();
            string appId = session.GetAppId();
            
            mLifecycleRegistry.AddInitial(appId);
            
            response.appId = appId;
            string sessionId = GeneratedSessionId();

            mAppRegistry.Add(sessionId, session);
            mAppSessionIdRegistry.Add(appId, sessionId);
            mContentPartnerIdRegistry.Add(appId, session.GetCatalog());
            response.sessionId = sessionId;
            if( !session.IsInactive()) {
                response.activeSessionId = GeneratedSessionId();
            }      
            
            response.loadedSessionId = GeneratedSessionId();
            
            response.transitionPending = false;

          
            // Lock and update session
            SessionInfo sessionInfo = response.getSessionInfo();
            mSessionRegistry.Add(appId, sessionInfo);

            string strResponse;
            response.ToString(strResponse);
            uint32_t connectionId = channel.ChannelId();
            uint32_t requestId = channel.Sequence();
            mWsManager.SendMessageToConnection(connectionId, strResponse, requestId);

            return Core::ERROR_NONE;
	}

    uint32_t LaunchDelegate::SetSession(const Core::JSONRPC::Context& channel, const JSetSession& request) {
        string appId = request.GetAppId();
        LifecycleState state = request.GetState();

        LifecycleState previous{};
		updateAppLifeCycleState(appId, state, previous);
        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::OnRequestReady(const Core::JSONRPC::Context& channel, const JListenRequest& request) {
        return HandleProviderRegistration(channel, ".onRequestReady", request.Get());
    }

    uint32_t LaunchDelegate::OnRequestClose(const Core::JSONRPC::Context& channel, const JListenRequest& request ){
        return HandleProviderRegistration(channel, ".onRequestClose", request.Get());
    }

    uint32_t LaunchDelegate::OnRequestFinished(const Core::JSONRPC::Context& channel, const JListenRequest& request ){
        return HandleProviderRegistration(channel, ".onRequestFinished", request.Get());
    }

    uint32_t LaunchDelegate::OnRequestLaunch(const Core::JSONRPC::Context& channel, const JListenRequest& request ){
        return HandleProviderRegistration(channel, ".onRequestLaunch", request.Get());
    }

    uint32_t LaunchDelegate::OnSessionTransitionCompleted(const Core::JSONRPC::Context& channel, const JListenRequest& request ){
        return HandleProviderRegistration(channel, ".onSessionTransitionCompleted", request.Get());
    }

    uint32_t LaunchDelegate::OnSessionTransitionCanceled(const Core::JSONRPC::Context& channel, const JListenRequest& request ){
        return HandleProviderRegistration(channel, ".onSessionTransitionCanceled", request.Get());
    }

    uint32_t LaunchDelegate::AddContext(const Core::JSONRPC::Context& channel, const JsonObject& params) {
        LOGINFO("Inside context");
        string paramsStr;
        params.ToString(paramsStr);
        LOGINFO("Adding context %s", paramsStr.c_str());
        if (params.HasLabel("context")) {
            JsonObject context = params["context"].Object();
            // Check if context is set with deviceSessionId string
            if (context.HasLabel("deviceSessionId") && context["deviceSessionId"].IsSet() && context["deviceSessionId"].Content() == JsonValue::type::STRING) {
                string deviceSessionId = context["deviceSessionId"].String();
                LOGINFO("Adding context with deviceSessionId: %s", deviceSessionId.c_str());
                // Call FbMertrics to set the appUser session id
                if (mService != nullptr) {
                    auto fbMetrics = mService->QueryInterfaceByCallsign<Exchange::IFbMetrics>(FB_METRICS_CALLSIGN);
                    if (fbMetrics != nullptr) {
                        fbMetrics->SetAppUserSessionId(deviceSessionId);
                        fbMetrics->Release();
                    } else {
                        LOGERR("FbMetrics interface not found.");
                    }
                } else {
                    LOGERR("Service is not initialized.");
                }
            } else {
                LOGERR("deviceSessionId is missing or not a string");
            }
        } else {
            LOGERR("context is missing or not a string");
        }

        uint32_t connectionId = channel.ChannelId();
        uint32_t requestId = channel.Sequence();
        mWsManager.SendMessageToConnection(connectionId, "null", requestId);

        return Core::ERROR_NONE;
    }

    uint32_t LaunchDelegate::HandleProviderRegistration(const Core::JSONRPC::Context& channel, const string& event, const bool& listen) {
        JListenResponse response;
        string prefix = LIFECYCLE_MANAGEMENT_MODULE;
        response.event = prefix + event;
        response.listening = listen;
        string strResponse;
        response.ToString(strResponse);
        string eventKey = StringUtils::toLower(response.event);
        uint32_t connectionId = channel.ChannelId();
        uint32_t requestId = channel.Sequence();
        if (listen) {
            mProviderRegistry.Add(eventKey, connectionId, requestId);
        } else {
            mProviderRegistry.Remove(eventKey);
        }
        mWsManager.SendMessageToConnection(connectionId, strResponse, requestId);
        return Core::ERROR_NONE;
    }

    //*READY//
	Core::hresult LaunchDelegate::Ready(const Context& context, string& result ) 
	{
		LOGINFO("[ requestId=%d connectionId=%d appId=%s]",
				context.requestId, context.connectionId, context.appId.c_str());
		LifecycleState previous{};
        JsonObject object;
        object["appId"] = context.appId;
        DispatchProvider("LifecycleManagement.onRequestReady", object);
		updateAppLifeCycleState(context.appId, LifecycleState::FOREGROUND, previous);
        result = "null";
		return Core::ERROR_NONE;
	}
	//Close//
	Core::hresult LaunchDelegate::Close(const Context& context ,
			const string& payload, string& result )  
	{
		LOGINFO("[ requestId = %d connectionId=%d appId=%s payload=%s] ",
				context.requestId, context.connectionId, context.appId.c_str(), payload.c_str());
		LifecycleState previous{};
		updateAppLifeCycleState(context.appId, LifecycleState::UNLOADING, previous);
        JsonObject params;
        if (params.FromString(payload)) {
            string reason;
            if (ObjectUtils::HasStringEntry(params,"reason", reason)) {
                LOGINFO("[ requestId = %d connectionId=%d appId=%s reason=%s] ",
                        context.requestId, context.connectionId, context.appId.c_str(), reason.c_str());
                JsonObject object;
                object["appId"] = context.appId;
                object["reason"] = reason;
                DispatchProvider("LifecycleManagement.onRequestClose", object);
            }
        }
        
        result = "null";

		return Core::ERROR_NONE;
	}

	Core::hresult LaunchDelegate::Finished( const Context& context, string& result )  
	{
		LOGINFO("[ requestId=%d connectionId=%d appId=%s]",
				context.requestId, context.connectionId, context.appId.c_str());
		LifecycleState previous{};
		updateAppLifeCycleState(context.appId, LifecycleState::UNLOADING, previous);
        JsonObject object;
        object["appId"] = context.appId;
        DispatchProvider("LifecycleManagement.onRequestFinished", object);
        result = "null";
		return Core::ERROR_NONE;
	}

    Core::hresult LaunchDelegate::Authenticate( const string& sessionId , std::string& appId)
	{
		LOGINFO("[SessionId = %s] ",sessionId.c_str());
        if (!mAppRegistry.GetAppId(sessionId, appId)) {
            return Core::ERROR_GENERAL;
        }
        return Core::ERROR_NONE;
		
	}

	Core::hresult LaunchDelegate::GetIntent( const string& appId , std::string& intent ) 
	{
		LOGINFO("[ApplicationID = %s] ", appId.c_str());
		if (!mAppRegistry.GetAppIntent(appId, intent)) {
            return Core::ERROR_GENERAL;
        }
		return Core::ERROR_NONE;
	}

    Core::hresult LaunchDelegate::GetSessionId(const string& appId, string& sessionId)
    {
        if (!mAppSessionIdRegistry.Get(appId, sessionId)) {
            return Core::ERROR_GENERAL;
        }
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::GetContentPartnerId(const string& appId, string& contentPartnerId)
    {
        if (!mContentPartnerIdRegistry.Get(appId, contentPartnerId)) {
            return Core::ERROR_GENERAL;
        }
        return Core::ERROR_NONE;
    }

    void LaunchDelegate::initializeWebsocketServer()
	{
        LOGINFO("Initializing Websocket server");
		WebSocketConnectionManager::Config config(LAUNCHDELEGATE_SOCKET_ADDRESS);
        std::string configLine = mService->ConfigLine();
        Core::OptionalType<Core::JSON::Error> error;

        if (config.FromString(configLine, error) == false)
        {
            LOGERR("Failed to parse config line, error: '%s', config line: '%s'.",
                    (error.IsSet() ? error.Value().Message().c_str() : "Unknown"),
                    configLine.c_str());
        }

		LOGINFO("Connector: %s", config.Connector.Value().c_str());
        Core::NodeId source(config.Connector.Value().c_str());
        LOGINFO("Parsed port: %d", source.PortNumber());

        mWsManager.SetAuthHandler(
                [this](const uint32_t connectionId, const std::string &token) -> bool
                {
                    string appId = Utils::ResolveQuery(token, "appId");
                    if (appId.empty())
                    {
                        // AS/AI calls without token
                        LOGDBG("No session token provided");
                    }
                    Core::IWorkerPool::Instance().Submit(ConnectionStatusNotificationJob::Create(this, connectionId, appId, true));
                    return true;
                }
            );

		mWsManager.SetMessageHandler(
            [this](const std::string& method, const std::string& params, const uint32_t requestId, const uint32_t connectionId)
            {
            LOGINFO("Received message: method=%s, params=%s, requestId=%d, connectionId=%d",
                    method.c_str(), params.c_str(), requestId, connectionId);       

            // Call Thunder plugin directly using resolver
            std::string realMethod = StringUtils::extractMethodName(method);
            if (realMethod.empty()) {
                std::string resolution;
                ErrorUtils::CustomBadMethod("Invalid method format",resolution);
                LOGERR("Invalid method format: %s", method.c_str());
                mWsManager.SendMessageToConnection(connectionId, resolution, requestId);
                return;
            }
            LOGDBG("Extracted method name: %s", realMethod.c_str());
            Core::IWorkerPool::Instance().Submit(ResolveJob::Create(this, connectionId, requestId, method, params));
        });


         mWsManager.SetDisconnectHandler(
                [this](const uint32_t connectionId)
                {
                    LOGINFO("Websocket disconnected: connectionId=%d", connectionId);
                    mProviderRegistry.CleanupByConnectionId(connectionId);

                    if (mAppNotifications != nullptr) {
                        if (Core::ERROR_NONE != mAppNotifications->Cleanup(connectionId, INTERNAL_GATEWAY_CALLSIGN)) {
                            LOGERR("AppNotifications Cleanup failed for connectionId: %d", connectionId);
                        }
                    }

                    mProviderDelegate.Cleanup(connectionId, INTERNAL_GATEWAY_CALLSIGN);

                    Core::IWorkerPool::Instance().Submit(ConnectionStatusNotificationJob::Create(this, connectionId, "", false));
                }
            );

        LOGINFO("Parsed port: %d", source.PortNumber());
        mWsManager.Start(source);
    }

    void LaunchDelegate::InternalResolve(const uint32_t connectionId, const uint32_t requestId, const string& method, const string& params) {
        std::string resolution;
        std::string lowerCaseMethod = StringUtils::toLower(method);
        
        if (StringUtils::checkStartsWithCaseInsensitive(lowerCaseMethod, LIFECYCLE_MANAGEMENT_MODULE)
            || StringUtils::checkStartsWithCaseInsensitive(lowerCaseMethod, METRICS_MANAGEMENT_MODULE) ) {
            auto result = Invoke(connectionId, requestId, "", lowerCaseMethod, params, resolution);
            LOGINFO("Final result=%d resolution: %s method: %s", result, resolution.c_str(), lowerCaseMethod.c_str());
            // For async requests Invocation returns -1 by default. Async are marked by method which have Core::JSON::Context as the first param
            // For these cases and ones which just return Core::ERROR_NONE we do not send a response back
            if (result != -1 && result != Core::ERROR_NONE ) {
                 ErrorUtils::CustomInternal("Internal Error", resolution);
                 mWsManager.SendMessageToConnection(connectionId, resolution, requestId);
            }
        } else {
            if (mAppGateway == nullptr) {
                mAppGateway = mService->QueryInterfaceByCallsign<Exchange::IAppGatewayResolver>(APP_GATEWAY_CALLSIGN);
                if (mAppGateway==nullptr) {
                    LOGERR("AppGateway interface not available");
                    ErrorUtils::NotAvailable(resolution);
                    return;
                }
            }

            if (mAppGateway != nullptr) {
                Context gatewayContext = {
                requestId,
                connectionId,
                "" // appId is not known here
                };
                std::string resolution;
                Core::hresult result = mAppGateway->Resolve(gatewayContext, INTERNAL_GATEWAY_CALLSIGN, method, params, resolution);
                if (!resolution.empty()) {
                LOGERR("AppGateway No Resolution found");
                ErrorUtils::CustomInternal("Internal Resolution Error", resolution);
                }
            }
                
        }
    }

    void LaunchDelegate::updateAppLifeCycleState(const std::string& appId, const LifecycleState& newState,
			LifecycleState& previous)
	{
        if (!appId.empty()) {
            if (!mLifecycleRegistry.UpdateLifecycle(appId, newState, previous)) {
                LOGERR("Failed to update lifecycle state for appId=%s", appId.c_str());
                return;
            }
            Core::IWorkerPool::Instance().Submit(DispatchLifecycleJob::Create(this, appId, newState, previous));
        }
    }

    void LaunchDelegate::DispatchLifecycleChange(const std::string& appId, const LifecycleState& newState,
			const LifecycleState& previous) {

            LOGINFO("AppId=%s lifecycle state updated to %d", appId.c_str(), newState);
            bool triggerNavigateTo = false;
            LifecycleChange change;
            change.state = newState;
            change.previous = previous;
            string event;
            switch (newState) {
                case LifecycleState::FOREGROUND:
                    event = "Lifecycle.onForeground";
                    triggerNavigateTo = true;
                break;
                case LifecycleState::BACKGROUND:
                    event = "Lifecycle.onBackground";
                    triggerNavigateTo = true;
                break;
                case LifecycleState::INACTIVE:
                    event = "Lifecycle.onInactive";
                break;
                case LifecycleState::SUSPENDED:
                    event = "Lifecycle.onSuspended";
                break;
                case LifecycleState::UNLOADING:
                    event = "Lifecycle.onUnloading";
                break;
                default:
                    LOGTRACE("Not valid for dispatch");
                break;
            }

            if (!SetupAppNotifications()) {
                 LOGERR("AppNotifications interface not available");
                 return;
            }

            if (mAppNotifications != nullptr) {
                string payload;
                change.ToString(payload);
            
                if ( Core::ERROR_NONE != mAppNotifications->Emit(event, payload, appId)) {
                    LOGERR("Error dispatching lifecycle notifications");
                }

                if (triggerNavigateTo) {
                    // Also trigger navigateTo event if going to foreground or background
                    string intent;
                    if (mAppRegistry.GetAppIntent(appId, intent)) {
                        JsonObject intentJson;
                        if(intentJson.FromString(intent)) {

                            string navigateToPayload;
                            intentJson.ToString(navigateToPayload);
                            if ( Core::ERROR_NONE != mAppNotifications->Emit("Discovery.onNavigateTo", navigateToPayload, appId)) {
                                LOGERR("Error dispatching navigateTo notifications");
                            }
                        }
                    } else {
                        LOGERR("Intent Not available for appId=%s", appId.c_str());
                    }
                }
            }

            auto fbMetrics = mService->QueryInterfaceByCallsign<Exchange::IFbMetrics>(FB_METRICS_CALLSIGN);
            if (fbMetrics != nullptr) {
                string previousStateStr(ToString(previous));
                string newStateStr(ToString(newState));
                if (Core::ERROR_NONE != fbMetrics->SetLifeCycle({appId}, newStateStr, previousStateStr)) {
                    LOGERR("Error sending lifecycle change to FbMetrics");
                }
                fbMetrics->Release();
            }

            if (newState == LifecycleState::UNLOADING) {
                // Remove session and content partner id mappings
                mAppSessionIdRegistry.Remove(appId);
                mContentPartnerIdRegistry.Remove(appId);
                mSessionRegistry.Remove(appId);
                mLifecycleRegistry.Remove(appId);
            }
    }

    void LaunchDelegate::DispatchProvider(const string& event, const JsonObject& params) {
        string eventKey = StringUtils::toLower(event);
        ProviderInfo provider = mProviderRegistry.Get(eventKey);
        JsonObject data;
        data["parameters"] = params;
        data["correlationId"] = GeneratedSessionId();

        string dataStr;
        data.ToString(dataStr);
        if (provider.channelId != static_cast<uint32_t>(-1) && provider.requestId != static_cast<uint32_t>(-1)) {
            mWsManager.SendMessageToConnection(provider.channelId, dataStr, provider.requestId);
        } else {
            LOGERR("No provider registered for event: %s", event.c_str());
        }
    }

    Core::hresult LaunchDelegate::Respond(const Context &context, const string &payload)
    {
        LOGINFO("Call LaunchDelegate::Respond");
        Core::IWorkerPool::Instance().Submit(RespondJob::Create(this, context.connectionId, context.requestId, payload));
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::Emit(const Context& context /* @in */, 
                const string& method /* @in */, const string& payload /* @in @opaque */) {
            Core::IWorkerPool::Instance().Submit(EmitJob::Create(this, context.connectionId, method, payload));
            return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::Request(const uint32_t connectionId /* @in */, 
            const uint32_t id /* @in */, const string& method /* @in */, const string& params /* @in @opaque */) {
        Core::IWorkerPool::Instance().Submit(RequestJob::Create(this, connectionId, id, method, params));
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::GetGatewayConnectionContext(const uint32_t connectionId /* @in */,
                const string& contextKey /* @in */, 
                 string &contextValue /* @out */) {
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::HandleAppEventNotifier(const string& event, const bool& listen, bool& status /* @out */) {
        // By default Launch delegate is implemented to Emit Events to AppNotifications for every update for any app
        status = true;
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::State( const Context& context /* @in */ ,string& state  /* @out */) {
        // Use StringToLifecycleState and get the lifecycle state
        LifecycleInfo lifecycleInfo;
        if (!mLifecycleRegistry.Get(context.appId, lifecycleInfo)) {
            return Core::ERROR_GENERAL;
        } else {
            // Convert LifecycleState to string
            for (const auto& pair : StringToLifecycleState) {
                if (pair.second == lifecycleInfo.state) {
                    state = pair.first;
                    break;
                }
            }
        }
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::Initialization(const Context& context, string& response) {
        Exchange::IFbPrivacy *privacyService = mService->QueryInterfaceByCallsign<Exchange::IFbPrivacy>(FB_PRIVACY_CALLSIGN);
        JsonObject responseJson;

        // Get Lmt and us_privacy values
        if (privacyService!= nullptr) {

            bool adContentTargetSettings;
            
            if (Core::ERROR_NONE == privacyService->AllowAppContentAdTargeting(adContentTargetSettings)) {
                if (adContentTargetSettings) {
                    responseJson["lmt"] = "0";
                    responseJson["us_privacy"] = "1-N-"; 
                } else {
                    responseJson["lmt"] = "1";
                    responseJson["us_privacy"] = "1-Y-";
                }
            }

            privacyService->Release();
            privacyService = nullptr;

        } else {
            LOGERR("FbPrivacy interface not available");
        }

        
        // Get last known intent
        string intent;
        if (mAppRegistry.GetAppIntent(context.appId, intent)) {
            JsonObject intentJson;
            if(intentJson.FromString(intent)) {
                JsonObject discoveryJson;
                discoveryJson["navigateTo"] = intentJson;
                responseJson["discovery"] = discoveryJson;
            }
        } else {
            LOGERR("Intent Not available for appId=%s", context.appId.c_str());
        }

        responseJson.ToString(response);

        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::HandleAppGatewayRequest(const Exchange::GatewayContext &context /* @in */,
                                          const string& method /* @in */,
                                          const string &payload /* @in @opaque */,
                                          string& result /*@out @opaque */)
        {
            SetupAppGatewayResponder();
            LOGINFO("HandleAppGatewayRequest: method=%s, payload=%s, appId=%s",
                    method.c_str(), payload.c_str(), context.appId.c_str());
            std::string lowerCaseMethod = StringUtils::toLower(method);
            if (StringUtils::checkStartsWithCaseInsensitive(lowerCaseMethod, INTEGRATED_PLAYER)) {
                return mProviderDelegate.HandleAppGatewayRequest(context, method, payload, APP_GATEWAY_CALLSIGN, result);
            }
            else if (lowerCaseMethod == "lifecycle.ready"){
                return Ready(context, result);
            }
            else if (lowerCaseMethod == "lifecycle.close") {
                return Close(context, payload, result);
            }
            else if (lowerCaseMethod == "lifecycle.finished") {
                return Finished(context, result);
            }
            else if (lowerCaseMethod == "lifecycle.state") {
                return State(context, result);
            }
            else if (lowerCaseMethod == "parameters.initialization") {
                return Initialization(context, result);
            }
        return Core::ERROR_NONE;
    }

    Core::hresult LaunchDelegate::Register(Exchange::IAppGatewayResponder::INotification *notification)
        {
            ASSERT (nullptr != notification);

            Core::SafeSyncType<Core::CriticalSection> lock(mConnectionStatusImplLock);

            /* Make sure we can't register the same notification callback multiple times */
            if (std::find(mConnectionStatusNotification.begin(), mConnectionStatusNotification.end(), notification) == mConnectionStatusNotification.end())
            {
                LOGINFO("Register notification");
                mConnectionStatusNotification.push_back(notification);
                notification->AddRef();
            }

            return Core::ERROR_NONE;
        }

        Core::hresult LaunchDelegate::Unregister(Exchange::IAppGatewayResponder::INotification *notification )
        {
            Core::hresult status = Core::ERROR_GENERAL;

            ASSERT (nullptr != notification);

            Core::SafeSyncType<Core::CriticalSection> lock(mConnectionStatusImplLock);

            /* Make sure we can't unregister the same notification callback multiple times */
            auto itr = std::find(mConnectionStatusNotification.begin(), mConnectionStatusNotification.end(), notification);
            if (itr != mConnectionStatusNotification.end())
            {
                (*itr)->Release();
                LOGINFO("Unregister notification");
                mConnectionStatusNotification.erase(itr);
                status = Core::ERROR_NONE;
            }
            else
            {
                LOGERR("notification not found");
            }

            return status;
        }

        void LaunchDelegate::OnConnectionStatusChanged(const string& appId, const uint32_t& connectionId, const bool& connected)
        {
            Core::SafeSyncType<Core::CriticalSection> lock(mConnectionStatusImplLock);
            for (auto& notification : mConnectionStatusNotification)
            {
                notification->OnAppConnectionChanged(appId, connectionId, connected);
            }
        }

        void LaunchDelegate::initializeDelegates() {
            mProviderDelegate.SetRespondCallback(
                [this](const ProviderContext& context, const std::string& payload)
                {
                    Context launchContext = {
                        context.requestId,
                        context.connectionId,
                        context.appId
                    };
                    if (ContextUtils::IsOriginGateway(context.origin)) {
                        if (!SetupAppGatewayResponder()) {
                            LOGERR("Failed to setup AppGatewayResponder");
                        } else {
                            this->mAppGatewayResponder->Respond(launchContext, payload);
                        }
                    } else {
                        this->Respond(launchContext, payload);
                    }
                }
            );

            mProviderDelegate.SetEmitCallback(
                [this](const string& event, const std::string& payload, const std::string& appId)
                {
                    if (!SetupAppNotifications()) {
                        LOGERR("Failed to setup AppNotifications");
                    } else {
                        this->mAppNotifications->Emit(event, payload, appId);
                    }
                }
            );
        }

        bool LaunchDelegate::SetupAppGatewayResponder() {
            if (mAppGatewayResponder == nullptr) {
                mAppGatewayResponder = mService->QueryInterfaceByCallsign<Exchange::IAppGatewayResponder>(APP_GATEWAY_CALLSIGN);
                if (mAppGatewayResponder == nullptr) {
                    LOGERR("AppGatewayResponder interface not available");
                    return false;
                } else {
                    // first time setup the notification listener
                    mAppGatewayResponder->Register(&mAppGatewayConnectionChangedSink);
                }
            }
            return true;
        }

        bool LaunchDelegate::SetupAppNotifications() {
            if (mAppNotifications == nullptr) {
                mAppNotifications = mService->QueryInterfaceByCallsign<Exchange::IAppNotifications>(APP_NOTIFICATIONS_CALLSIGN);
                if (mAppNotifications == nullptr) {
                    LOGERR("AppNotifications interface not available");
                    return false;
                }
            }
            return true;
        }

        Core::hresult LaunchDelegate::CheckPermissionGroup(const string& appId /* @in */,
                                                       const string& permissionGroup /* @in */,
                                                       bool& allowed /* @out */)  {

            allowed = true;
            return Core::ERROR_NONE;
        }
    
} // namespace Plugin
} // namespace WPEFramework
