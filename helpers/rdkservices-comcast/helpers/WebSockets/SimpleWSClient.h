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

#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

namespace WPEFramework {
namespace Plugin {
namespace WebSockets {

/**
 * @brief A synchronous WebSocket client.
 *
 * This client is designed to establish a connection to a WebSocket server via a specified URI,
 * send a request, and wait for the response. It is intended for simple, one-time communications
 * with a WebSocket server. After receiving the first message from the server, the client stops
 * and closes the connection.
 */
class SimpleWSClient {
public:
    SimpleWSClient();

    /**
     * @brief Connects to a WebSocket server and sends a request.
     *
     * Attempts to establish a WebSocket connection to the specified URI. This method blocks until the connection either
     * succeeds, fails, or a response is received.
     *
     * @param uri The URI of the WebSocket server.
     * @return True if connection and request sending are successful, false otherwise.
     */
    bool connectAndSendRequest(const std::string &uri);

    /**
     * @brief Retrieves the server's response.
     *
     * Waits for and returns the server's response. If an error occurs, an empty string is returned.
     * This method should be called after `connectAndSendRequest`.
     *
     * @return The server's response as a string, or an empty string on timeout/error.
     */
    std::string getResponse();

private:
    void onOpen(websocketpp::connection_hdl hdl);
    void onFail(websocketpp::connection_hdl hdl);
    void onMessage(websocketpp::connection_hdl hdl,
                   websocketpp::client<websocketpp::config::asio_client>::message_ptr msg);

    websocketpp::client<websocketpp::config::asio_client> m_client;
    std::string m_response;
    bool m_messageReceived;
    bool m_hasError;
    std::mutex m_mutex;
    std::condition_variable m_condVar;
};

} // namespace WebSockets
} // namespace Plugin
} // namespace WPEFramework