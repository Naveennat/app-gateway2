/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2021 RDK Management
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

#include "WSEndpoint.h"
#include "CommunicationInterface/CommandInterface.h"
#include "CommunicationInterface/JsonRpcInterface.h"
#include "../utils.h"

#include "Roles/SingleClientServer.h"
#include "Roles/Client.h"
#include "PingPong/PingPongEnabled.h"
#include "PingPong/PingPongDisabled.h"
#include "Encryption/MTlsEnabled.h"
#include "Encryption/NoEncryption.h"

namespace WPEFramework {
namespace Plugin       {
namespace WebSockets   {

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::WSEndpoint(const char *name)
{
    LOGINFO();
    // Uncomment for connection details
    // endpointImpl_.set_access_channels(websocketpp::log::alevel::all);
    // endpointImpl_.set_error_channels(websocketpp::log::elevel::all);
    endpointImpl_.clear_access_channels(websocketpp::log::alevel::all);
    endpointImpl_.clear_error_channels(websocketpp::log::elevel::all);

    registerHandlers();

    endpointImpl_.init_asio();
    Encryption<WSEndpoint, Role<WSEndpoint> >::setup();
    endpointImpl_.start_perpetual();

    name_ = name;
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::~WSEndpoint()
{
    LOGINFO("(%s)", name());
    endpointImpl_.stop_perpetual();
    closeConnection();
    stopEventLoop();
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::closeConnection()
{
    LOGINFO("(%s)", name());
    auto conn = getConnection(connectionHandler_);
    if (!conn)
    {
        LOGINFO("Cant get connection (Probably connection already closed).");
        return;
    }
    if (conn->get_state() != websocketpp::session::state::open)
    {
        LOGINFO("Can't close connection which is not open.");
        return;
    }

    websocketpp::lib::error_code ec;
    endpointImpl_.close(connectionHandler_, websocketpp::close::status::going_away, "", ec);
    if (ec) {
        LOGERR("Closing connection failed, reason: %s", ec.message().c_str());
    }
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::startEventLoop()
{
    LOGINFO("%s", name());
    if (!eventLoopThread_)
    {
        LOGINFO("Starting new event loop thread");
        eventLoopThread_ = std::thread([this]() {
            LOGINFO("Starting websocket communication on new thread");
            endpointImpl_.run();
            LOGINFO("Event loop thread finished");
        });
    }
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::stopEventLoop()
{
    LOGINFO("%s", name());
    if (eventLoopThread_ && eventLoopThread_->joinable())
    {
        LOGINFO("Event loop thread is joinable. Waiting for join.");
        eventLoopThread_->join();
        eventLoopThread_ = boost::none;
    }
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::registerHandlers()
{
    LOGINFO("(%s)", name());
    endpointImpl_.set_message_handler(std::bind(&WSEndpoint::onMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
    endpointImpl_.set_open_handler(std::bind(&WSEndpoint::onOpen, this,  websocketpp::lib::placeholders::_1));
    endpointImpl_.set_close_handler(std::bind(&WSEndpoint::onClose, this,  websocketpp::lib::placeholders::_1));
    endpointImpl_.set_fail_handler(std::bind(&WSEndpoint::onFail, this, websocketpp::lib::placeholders::_1));
    endpointImpl_.set_open_handshake_timeout(5000);
    endpointImpl_.set_close_handshake_timeout(5000);
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
bool WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::send(const std::string& message)
{
    if (message.empty())
    {
        LOGERR("(%s )Can't send empty message", name());
        return false;
    }

    websocketpp::lib::error_code ec;
    endpointImpl_.send(connectionHandler_, message, websocketpp::frame::opcode::text, ec);
    if (ec)
    {
        LOGERR("(%s) Sending failed, reason: %s", name(), ec.message().c_str());
        return false;
    }

    return true;
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::onMessage(ConnectionHandler, typename WebsocketppEndpoint::message_ptr msg)
{
    if (msg->get_opcode() != websocketpp::frame::opcode::text)
    {
        LOGERR("(%s) Received message is not tagged with text opcode, droping.", name());
        return;
    }
    MessagingInterface<WSEndpoint>::onMessage(msg->get_payload());
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::onOpen(ConnectionHandler handler)
{
    LOGINFO("(%s) New connection opened.", name());
    connectionHandler_ = handler;
    connectionInitializationCallback_(ConnectionInitializationResult(true));
    PingPong<WSEndpoint>::startPing(handler);
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::onFail(ConnectionHandler handler)
{
    LOGINFO("(%s) Connection attempt failure.", name());

    const auto& connection = getConnection(handler);
    const auto& connectionEc = connection->get_ec();
    if (connectionEc)
    {
        LOGINFO("Connection error: %s. Category: %s. Value: %d", connectionEc.message().c_str(),
            connectionEc.category().name(), connectionEc.value());
    }

    ConnectionInitializationResult result(false);
    Encryption<WSEndpoint, Role<WSEndpoint> >::setAuthenticationState(result, handler);

    connectionInitializationCallback_(result);
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
void WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::onClose(ConnectionHandler)
{
    LOGINFO("(%s) Connection closed.", name());
    connectionClosedCallback_();
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
typename WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::ConnectionPtr
WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::getConnection(ConnectionHandler handler)
{
    websocketpp::lib::error_code handlerToConnectionEc;
    auto connection = endpointImpl_.get_con_from_hdl(handler, handlerToConnectionEc);
    if (handlerToConnectionEc)
    {
        LOGERR("Can't get connection from handler, reason: %s", handlerToConnectionEc.message().c_str());
        return {};
    }
    return connection;
}

template<
    template <typename> typename Role,
    template <typename> typename MessagingInterface,
    template <typename> typename PingPong,
    template <typename, typename> typename Encryption
>
const char *WSEndpoint<Role, MessagingInterface, PingPong, Encryption>::name() const
{
    return name_.c_str();
}

template class WSEndpoint<Client, CommandInterface, PingPongEnabled, MTlsEnabled>;
template class WSEndpoint<Client, JsonRpcInterface, PingPongEnabled, MTlsEnabled>;
template class WSEndpoint<Client, JsonRpcInterface, PingPongEnabled, NoEncryption>;
template class WSEndpoint<Client, JsonRpcInterface, PingPongDisabled, NoEncryption>;
template class WSEndpoint<SingleClientServer, CommandInterface, PingPongEnabled, NoEncryption>;
template class WSEndpoint<SingleClientServer, JsonRpcInterface, PingPongEnabled, NoEncryption>;
template class WSEndpoint<SingleClientServer, JsonRpcInterface, PingPongEnabled, MTlsEnabled>;

}   // namespace Websockets
}   // namespace WPEFramework
}   // namespace Plugin
