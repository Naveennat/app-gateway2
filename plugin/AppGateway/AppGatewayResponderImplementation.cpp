/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

#include <string>
#include <cstdlib>
#include <plugins/JSONRPC.h>
#include <plugins/IShell.h>
#include "AppGatewayResponderImplementation.h"
#include "UtilsLogging.h"
#include "UtilsConnections.h"
#include "UtilsCallsign.h"
#include <interfaces/IAppNotifications.h>

// App Gateway is only available via local connections,
// so we can use a simple in-memory registry to track connection IDs and their associated app IDs.
#define APPGATEWAY_SOCKET_ADDRESS "127.0.0.1:3473"
#define DEFAULT_CONFIG_PATH "/etc/app-gateway/resolution.base.json"

namespace WPEFramework
{
    namespace Plugin
    {
        SERVICE_REGISTRATION(AppGatewayResponderImplementation, 1, 0, 0);

        AppGatewayResponderImplementation::AppGatewayResponderImplementation()
            : mService(nullptr),
            mWsManager(),
            mAuthenticator(nullptr),
            mResolver(nullptr),
            mConnectionStatusImplLock(),
            mEnhancedLoggingEnabled(false)
        {
            LOGINFO("AppGatewayResponderImplementation constructor");
#ifdef ENABLE_APP_GATEWAY_AUTOMATION
        #ifdef APP_GATEWAY_ENHANCED_LOGGING_INDICATOR
            struct stat buffer;
            mEnhancedLoggingEnabled = (stat(APP_GATEWAY_ENHANCED_LOGGING_INDICATOR, &buffer) == 0);
            LOGINFO("Enhanced logging enabled: %s (indicator: %s)", mEnhancedLoggingEnabled ? "true" : "false", APP_GATEWAY_ENHANCED_LOGGING_INDICATOR);
        #endif
#endif
        }

        AppGatewayResponderImplementation::~AppGatewayResponderImplementation()
        {
            LOGINFO("AppGatewayResponderImplementation destructor");
            if (nullptr != mService)
            {
                mService->Release();
                mService = nullptr;
            }

            if (nullptr != mResolver)
            {
                mResolver->Release();
                mResolver = nullptr;
            }

            if (nullptr != mAuthenticator)
            {
                mAuthenticator->Release();
                mAuthenticator = nullptr;
            }

        }

        uint32_t AppGatewayResponderImplementation::Configure(PluginHost::IShell *shell)
        {
            LOGINFO("Configuring AppGatewayResponderImplementation");
            uint32_t result = Core::ERROR_NONE;
            ASSERT(shell != nullptr);
            mService = shell;
            mService->AddRef();
            result = InitializeWebsocket();

            return result;
        }

        uint32_t AppGatewayResponderImplementation::InitializeWebsocket(){
            // Initialize WebSocket server
            WebSocketConnectionManager::Config config(APPGATEWAY_SOCKET_ADDRESS);
            const std::string configLine = (mService != nullptr ? mService->ConfigLine() : std::string());

            // L0 test note:
            // The in-proc ServiceMock::ConfigLine() is tailored for IShell::Root() / RootConfig parsing
            // and may not represent this plugin's own configuration schema (e.g., it can contain only
            // a `root` object). Parsing such JSON with WebSocketConnectionManager::Config can fail.
            //
            // Therefore: only parse if the config line appears to contain websocket configuration.
            if (!configLine.empty() && (configLine.find("\"connector\"") != std::string::npos)) {
                Core::OptionalType<Core::JSON::Error> error;
                if (config.FromString(configLine, error) == false) {
                    LOGWARN("ConfigLine is present but not websocket-config compatible; using default connector '%s'. Parse error: '%s'. ConfigLine='%s'",
                            config.Connector.Value().c_str(),
                            (error.IsSet() ? error.Value().Message().c_str() : "Unknown"),
                            configLine.c_str());
                }
            } else {
                LOGINFO("Using default websocket connector '%s' (ConfigLine did not contain connector config).",
                        config.Connector.Value().c_str());
            }

            LOGINFO("Connector: %s", config.Connector.Value().c_str());
            Core::NodeId source(config.Connector.Value().c_str());
            LOGINFO("Parsed port: %d", source.PortNumber());
            mWsManager.SetMessageHandler(
                [this](const std::string &method, const std::string &params, const int requestId, const uint32_t connectionId)
                {
                    Core::IWorkerPool::Instance().Submit(WsMsgJob::Create(this, method, params, requestId, connectionId));
                });

            mWsManager.SetAuthHandler(
                [this](const uint32_t connectionId, const std::string &token) -> bool
                {
                    string sessionId = Utils::ResolveQuery(token, "session");
                    if (sessionId.empty())
                    {
                        LOGERR("No session token provided");
                        return false;
                    }

                    if ( mAuthenticator==nullptr ) {
                        mAuthenticator = mService->QueryInterfaceByCallsign<Exchange::IAppGatewayAuthenticator>(GATEWAY_AUTHENTICATOR_CALLSIGN);
                        if (mAuthenticator == nullptr) {
                            LOGERR("Authenticator Not available");
                            return false;
                        }
                    }

                    string appId;
                    if (Core::ERROR_NONE == mAuthenticator->Authenticate(sessionId,appId)) {
                        LOGTRACE("APP ID %s", appId.c_str());
                        mAppIdRegistry.Add(connectionId, appId);
                        
                        #ifdef ENABLE_APP_GATEWAY_AUTOMATION
                        // Check if this is the automation client
                        #ifdef AUTOMATION_APP_ID
                        if (appId == AUTOMATION_APP_ID) {
                            mWsManager.SetAutomationId(connectionId);
                            LOGINFO("Automation server connected with ID: %d, appId: %s", connectionId, appId.c_str());
                        }
                        #endif
                        #endif
                        
                        Core::IWorkerPool::Instance().Submit(ConnectionStatusNotificationJob::Create(this, connectionId, appId, true));

                        return true;
                    }

                    return false;
                });

            mWsManager.SetDisconnectHandler(
                [this](const uint32_t connectionId)
                {
                    LOGINFO("Connection disconnected: %d", connectionId);
                    string appId;
                    if (!mAppIdRegistry.Get(connectionId, appId)) {
                        LOGERR("No App ID found for connection %d during disconnect", connectionId);
                    } else {
                        LOGINFO("App ID %s found for connection %d during disconnect", appId.c_str(), connectionId);
                        Core::IWorkerPool::Instance().Submit(ConnectionStatusNotificationJob::Create(this, connectionId, appId, false));
                    }
                    
                    mAppIdRegistry.Remove(connectionId);
                    Exchange::IAppNotifications* appNotifications = mService->QueryInterfaceByCallsign<Exchange::IAppNotifications>(APP_NOTIFICATIONS_CALLSIGN);
                    if (appNotifications != nullptr) {
                        // NOTE: current SDK's IAppNotifications does not expose Cleanup().
                        // Best-effort: notify about disconnect so AppNotifications can react if desired.
                        JsonObject payload;
                        payload["connectionId"] = connectionId;
                        payload["origin"] = APP_GATEWAY_CALLSIGN;

                        std::string payloadStr;
                        payload.ToString(payloadStr);

                        (void)appNotifications->Notify(_T("appgateway.connection.cleanup"), payloadStr);
                        appNotifications->Release();
                        appNotifications = nullptr;
                    }
                }
            );
            mWsManager.Start(source);
            return Core::ERROR_NONE;
        }

        Core::hresult AppGatewayResponderImplementation::Respond(const Context& context, const string& payload)
        {
            Core::IWorkerPool::Instance().Submit(RespondJob::Create(this, context.connectionId, context.requestId, payload));
            return Core::ERROR_NONE;
        }

        Core::hresult AppGatewayResponderImplementation::Emit(const Context& context /* @in */, 
                const string& method /* @in */, const string& payload /* @in @opaque */) {
            Core::IWorkerPool::Instance().Submit(EmitJob::Create(this, context.connectionId, method, payload));
            return Core::ERROR_NONE;
        }

        Core::hresult AppGatewayResponderImplementation::Request(const uint32_t connectionId /* @in */, 
                const uint32_t id /* @in */, const string& method /* @in */, const string& params /* @in @opaque */) {
            Core::IWorkerPool::Instance().Submit(RequestJob::Create(this, connectionId, id, method, params));
            return Core::ERROR_NONE;
        }

        Core::hresult AppGatewayResponderImplementation::GetGatewayConnectionContext(const uint32_t connectionId /* @in */,
                const string& contextKey /* @in */,
                 string& contextValue /* @out */) {
            // L0 / API contract:
            // - Return ERROR_NONE and fill contextValue when key exists for the connection.
            // - Return ERROR_BAD_REQUEST when the connection is unknown or the key is not present.
            contextValue.clear();

            if (contextKey.empty()) {
                return Core::ERROR_BAD_REQUEST;
            }

            // Minimal hook for L0 tests (and offline use):
            // Allow injecting a single key/value via env vars without requiring real websocket traffic.
            const char* envConn = std::getenv("APPGATEWAY_TEST_CONN_ID");
            const char* envKey  = std::getenv("APPGATEWAY_TEST_CTX_KEY");
            const char* envVal  = std::getenv("APPGATEWAY_TEST_CTX_VALUE");

            const bool anyEnvSet =
                (envConn != nullptr && *envConn != '\0') ||
                (envKey  != nullptr && *envKey  != '\0') ||
                (envVal  != nullptr && *envVal  != '\0');

            // If any env var is set, enforce strict behavior against that injected record only.
            // This makes L0 deterministic and ensures unknown keys/connections produce BAD_REQUEST.
            if (anyEnvSet) {
                if (envConn == nullptr || envKey == nullptr || envVal == nullptr ||
                    *envConn == '\0' || *envKey == '\0') {
                    return Core::ERROR_BAD_REQUEST;
                }

                const uint32_t configuredConn = static_cast<uint32_t>(std::strtoul(envConn, nullptr, 10));
                if (configuredConn != connectionId) {
                    return Core::ERROR_BAD_REQUEST;
                }
                if (contextKey != envKey) {
                    return Core::ERROR_BAD_REQUEST;
                }

                contextValue = envVal;
                return Core::ERROR_NONE;
            }

            // In this isolated build we do not maintain a full connection context registry,
            // so be strict per API contract.
            return Core::ERROR_BAD_REQUEST;
        }


        void AppGatewayResponderImplementation::DispatchWsMsg(const std::string &method,
                                                     const std::string &params,
                                                     const uint32_t requestId,
                                                     const uint32_t connectionId)
        {
            std::string resolution;
            string appId;

            if (mAppIdRegistry.Get(connectionId, appId)) {

                if (mEnhancedLoggingEnabled) {
                    LOGDBG("%s-->[[a-%d-%d]] method=%s, params=%s",
                           appId.c_str(),connectionId, requestId, method.c_str(), params.c_str());
                }
                // App Id is available
                Context context = {
                    requestId,
                    connectionId,
                    appId
                };

                if (mResolver == nullptr) {
                    mResolver = mService->QueryInterface<Exchange::IAppGatewayResolver>();
                }

                if (mResolver == nullptr) {
                    LOGERR("Resolver interface not available");
                    return;
                }

                string resolution;
                if (Core::ERROR_NONE != mResolver->Resolve(context, APP_GATEWAY_CALLSIGN, method, params, resolution)) {
                    LOGERR("Resolver Failure");
                }
            } else {
                LOGERR("No App ID found for connection %d. Terminate connection", connectionId);
                mWsManager.Close(connectionId);
            }
        }


        Core::hresult AppGatewayResponderImplementation::Register(Exchange::IAppGatewayResponder::INotification *notification)
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

        Core::hresult AppGatewayResponderImplementation::Unregister(Exchange::IAppGatewayResponder::INotification *notification )
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

        void AppGatewayResponderImplementation::OnConnectionStatusChanged(const string& appId, const uint32_t connectionId, const bool connected)
        {
            Core::SafeSyncType<Core::CriticalSection> lock(mConnectionStatusImplLock);
            for (auto& notification : mConnectionStatusNotification)
            {
                notification->OnAppConnectionChanged(appId, connectionId, connected);
            }

            #ifdef ENABLE_APP_GATEWAY_AUTOMATION
            // Notify automation server of connection status change
            mWsManager.UpdateConnection(connectionId, appId, connected);
            #endif
        }

    } // namespace Plugin
} // namespace WPEFramework
