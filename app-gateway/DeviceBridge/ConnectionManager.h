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

#pragma once

#include <memory>
#include <mutex>
#include <atomic>
#include <algorithm>
#include <boost/core/noncopyable.hpp>

#include "Module.h"
#include "utils.h"

#include "MdnsUtils.h"
#include "WebSockets/WSEndpoint.h"
#include "WebSockets/PingPong/PingPongEnabled.h"
#include "WebSockets/Roles/Client.h"
#include "WebSockets/CommunicationInterface/JsonRpcInterface.h"
#include "WebSockets/JsonRpc/Notification.h"
#include "WebSockets/Encryption/MTlsEnabled.h"
#include "WebSockets/Encryption/NoEncryption.h"
#include "Processor.h"

#include "IConnectionStatusListener.h"
#include "UsbInterfaceMonitor.h"
#include <interfaces/IPowerManager.h>
#include "PowerManagerInterface.h"


using namespace WPEFramework::Exchange;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

namespace WPEFramework {
namespace Plugin       {

class ConnectionManager : public IMdnsBrowserListener
{
public:
    ConnectionManager() = default;
    virtual ~ConnectionManager() = default;
    void initialize(IConnectionStatusListener& listener);
    void deinitialize();
    bool isConnected() const;
    bool getConnectionStatus(const JsonObject& parameters, JsonObject& response);
    bool rpcCall(const std::string &method, const JsonObject &parameters, JsonObject &response);
    void setNotificationHandler(std::function<void(const WebSockets::JsonRpc::Notification&)> notificationHandler);
    void powerModeChange(const PowerState mode);

private:
    ConnectionManager(const ConnectionManager &) = delete;
    ConnectionManager &operator=(const ConnectionManager &) = delete;

    void startMdns();
    void restartMdns();
    void stopMdns();
    void resetMdns();

    void connect();
    void onServiceNew(std::string name, std::string type, std::string domain, std::string ip, uint16_t port) override;
    void onServiceRemove(std::string name, std::string type, std::string domain) override;
    void onServiceResolveFailure(std::string name, std::string type, std::string domain) override;
    void onServiceConnection(WebSockets::ConnectionInitializationResult result);
    void onServiceDisconnected();

    bool isIPv4Address(const std::string &address) const;
    void setDeviceAddress(const std::string &ipAddress, const std::string &port);
    std::string getInterfaceName();
    bool interfaceExists(std::string &ifname);
    void setConnectionStatus(bool connected);
    void setDeviceAuthenticatedState(boost::optional<bool> authenticated);
    void onUsbInterfaceDetectedChange(bool detected);
    void onSettledUsbInterfaceDetectedChange(bool detected);
    void updateUsbInterfaceDetectedState(bool detected);
    JsonObject serializeStatusToJson() const;
    void notifyListener();
    void restartMdnsWhenNetworkAvailable();

    bool hasStatusChanged(const JsonObject &status) const;
    void usbSettlingMonitor();

    std::unique_ptr<MdnsUtils> mdnsUtils_;
    std::mutex mdnsMutex;
    std::string deviceIPAddress_;
    std::string devicePort_;
    std::string deviceInterface_;
    std::atomic_bool deviceConnected_{false};
    std::atomic_bool pendingDisconnection_{false};
    int totalConnections{0};
    bool usbInterfaceDetected_{false};
    bool unsettledUsbInterfaceDetected_{false};
    bool isSettlingThreadWaiting_{false};
    std::mutex usbSettlingMutex;
    std::condition_variable usbSettlingCv;
    std::thread usbSettlingMonitorThread_;
    bool usbSettlingMonitorExit_{false};
    PowerState powerState_{WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF};
    boost::optional<bool> deviceAuthenticated_;
    std::string connectionStatusReason_;
    boost::optional<JsonObject> lastStatus_;
    IConnectionStatusListener *connectionStatusListener_;
    UsbInterfaceMonitor usbInterfaceMonitor_;
    std::string address_;
    WebSockets::WSEndpoint<
        WebSockets::Client,
        WebSockets::JsonRpcInterface,
        WebSockets::PingPongEnabled,
        WebSockets::MTlsEnabled
    > wsClient_{"DeviceBridge"};
    bool staticIpMode_{false};
    Processor processor_;
};

} // namespace Plugin
} // namespace WPEFramework
