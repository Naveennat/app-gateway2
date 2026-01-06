/**
 * If not stated otherwise in this file or this component's LICENSE
 * file the following copyright and licenses apply:
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
 **/

#include "SimpleWSClient.h"

#include "../helpers/utils.h"

namespace WPEFramework {
namespace Plugin {
namespace WebSockets {

SimpleWSClient::SimpleWSClient() : m_messageReceived(false), m_hasError(false) {
    m_client.init_asio();
    m_client.set_open_handler(bind(&SimpleWSClient::onOpen, this, websocketpp::lib::placeholders::_1));
    m_client.set_fail_handler(bind(&SimpleWSClient::onFail, this, websocketpp::lib::placeholders::_1));
    m_client.set_message_handler(
        bind(&SimpleWSClient::onMessage, this, websocketpp::lib::placeholders::_1, websocketpp::lib::placeholders::_2));
}

bool SimpleWSClient::connectAndSendRequest(const std::string &uri) {
    websocketpp::lib::error_code ec;
    auto con = m_client.get_connection(uri, ec);
    if (ec) {
        LOGERR("Could not create connection: %s", ec.message().c_str());
        m_hasError = true;
        return false;
    }

    m_client.connect(con);
    m_client.run();
    return !m_hasError;
}

std::string SimpleWSClient::getResponse() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_condVar.wait_for(lock, std::chrono::seconds(1), [this] { return m_messageReceived; })) {
        return m_response;
    } else {
        LOGERR("Timeout or error");
        return "";
    }
}

void SimpleWSClient::onOpen(websocketpp::connection_hdl) {}

void SimpleWSClient::onFail(websocketpp::connection_hdl) {
    LOGERR("Connection failed");
    m_hasError = true;
    m_condVar.notify_one();
}

void SimpleWSClient::onMessage(websocketpp::connection_hdl,
                               websocketpp::client<websocketpp::config::asio_client>::message_ptr msg) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_response = msg->get_payload();
    m_messageReceived = true;
    m_condVar.notify_one();
    m_client.stop();
}

} // namespace WebSockets
} // namespace Plugin
} // namespace WPEFramework