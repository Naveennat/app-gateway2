#pragma once
/*
 * Compatibility shim for AppGatewayResponderImplementation.
 *
 * Upstream uses a WebSocketConnectionManager implementation that is not vendored
 * into this isolated repository. For build validation and L0 in-proc harness,
 * we only require enough surface area to compile and to behave as a no-op.
 */
#include <cstdint>
#include <functional>
#include <string>

#include <core/JSON.h>

#include "../../WsManager.h"

// Provide the API expected by AppGatewayResponderImplementation while delegating to a minimal stub.
class WebSocketConnectionManager {
public:
    class Config : public WPEFramework::Core::JSON::Container {
    public:
        explicit Config(const std::string& endpoint = std::string("127.0.0.1:3473"))
            : WPEFramework::Core::JSON::Container()
            , Connector(endpoint.c_str())
        {
            Add(_T("connector"), &Connector);
        }

        WPEFramework::Core::JSON::String Connector;
    };

    using MessageHandler = std::function<void(const std::string& method,
                                              const std::string& params,
                                              const int requestId,
                                              const uint32_t connectionId)>;
    using AuthHandler = std::function<bool(const uint32_t connectionId, const std::string& token)>;
    using DisconnectHandler = std::function<void(const uint32_t connectionId)>;

    WebSocketConnectionManager() = default;

    void SetMessageHandler(MessageHandler handler) { _messageHandler = std::move(handler); }
    void SetAuthHandler(AuthHandler handler) { _authHandler = std::move(handler); }
    void SetDisconnectHandler(DisconnectHandler handler) { _disconnectHandler = std::move(handler); }

    void Start(const std::string& /*source*/)
    {
        // No-op: isolated build doesn't run real websocket server.
    }

    void Close(uint32_t connectionId) { _impl.Close(connectionId); }

    void SendMessageToConnection(uint32_t /*connectionId*/, const std::string& /*payload*/, int /*requestId*/) {}
    void DispatchNotificationToConnection(uint32_t /*connectionId*/, const std::string& /*payload*/, const std::string& /*designator*/) {}
    void SendRequestToConnection(uint32_t /*connectionId*/, const std::string& /*designator*/, uint32_t /*requestId*/, const std::string& /*payload*/) {}

    void SetAutomationId(uint32_t /*connectionId*/) {}
    void UpdateConnection(uint32_t /*connectionId*/, const std::string& /*appId*/, bool /*connected*/) {}

private:
    WsManager _impl;
    MessageHandler _messageHandler;
    AuthHandler _authHandler;
    DisconnectHandler _disconnectHandler;
};
