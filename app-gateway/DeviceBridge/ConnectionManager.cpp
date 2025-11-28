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

#include "ConnectionManager.h"
#include "../helpers/utils.h"

#include <chrono>
#include <ifaddrs.h>

#include "WebSockets/JsonRpc/Response.h"
#include "WebSockets/JsonRpc/Request.h"
#include "libIBusDaemon.h"

namespace WPEFramework
{
namespace Plugin
{

constexpr char CONFIG_PATH[] = "/etc/device.properties";
constexpr char REASON_BOOTING[] = "BOOTING";
constexpr char REASON_AUTHENTICATING[] = "AUTHENTICATING";
constexpr char REASON_INVALID_CERTIFICATE[] = "INVALID_CERTIFICATE";
constexpr char REASON_CONNECTED[] = "CONNECTED";
constexpr char REASON_DISCONNECTED[] = "DISCONNECTED";
constexpr int USB_SETTLING_TIMEOUT = 12000;

void ConnectionManager::initialize(IConnectionStatusListener& listener)
{
    LOGINFO();

    deviceInterface_ = getInterfaceName();
    LOGINFO("Device interface: %s", deviceInterface_.c_str());

    connectionStatusListener_ = &listener;
    connectionStatusReason_ = REASON_DISCONNECTED;
    usbSettlingMonitorThread_ = std::thread(&ConnectionManager::usbSettlingMonitor, this);

    // Wait for the usbSettlingMonitor thread to be read before proceeding, or risk missing the first USB event
    while(!isSettlingThreadWaiting_) std::this_thread::sleep_for(std::chrono::milliseconds(10));

    usbInterfaceMonitor_.initialize(std::bind(&ConnectionManager::onUsbInterfaceDetectedChange, this, std::placeholders::_1));
    usbInterfaceMonitor_.startMonitoring();
}

void ConnectionManager::startMdns()
{
    LOGINFO();
    std::lock_guard<std::mutex> lock(mdnsMutex);

    if (mdnsUtils_)
    {
        LOGWARN("Mdns already started.");
        return;
    }

    if(!interfaceExists(deviceInterface_))
    {
        LOGINFO("DeviceBridge interface (%s) not yet found, waiting for usb detection.", deviceInterface_.c_str());
        updateUsbInterfaceDetectedState(false);
        return;
    }

    mdnsUtils_.reset(new MdnsUtils());
    LOGINFO("mDNSResponder Browser registration.");
    if (!mdnsUtils_->registerBrowser("_cherry._tcp", this, deviceInterface_))
    {
        LOGERR("Failed to register mDNSResponder browser");
        return;
    }
}

void ConnectionManager::stopMdns()
{
    std::lock_guard<std::mutex> lock(mdnsMutex);
    if (mdnsUtils_)
        mdnsUtils_->stopBrowser();
}

void ConnectionManager::resetMdns()
{
    std::lock_guard<std::mutex> lock(mdnsMutex);
    mdnsUtils_.reset();
}

// Restarting needed because Mdns doesn't emit notifications about service changes properly.
// Applies both for browsing and resolving services.
void ConnectionManager::restartMdns()
{
    if(staticIpMode_ && !address_.empty())
    {
        LOGINFO("Safe/Static ip mode on. Using last known URI to connect once again.");
        connect();
    }
    else
    {
        resetMdns();
        LOGINFO("Mdns destroyed.");
        startMdns();
    }
}

void ConnectionManager::deinitialize()
{
    LOGINFO();
    resetMdns();
    usbInterfaceMonitor_.deinitialize();
    if(usbSettlingMonitorThread_.joinable())
    {
        usbSettlingMonitorExit_ = true;
        usbSettlingMutex.lock();
        usbSettlingMutex.unlock();
        usbSettlingCv.notify_one();
        usbSettlingMonitorThread_.join();
    }
}

void ConnectionManager::onServiceNew(std::string name, std::string type, std::string domain, std::string ip, uint16_t port)
{
    LOGINFO("New service found (or old service resolved one more time) name:type:domain:ip:port %s:%s:%s:%s:%u",
        name.c_str(), type.c_str(), domain.c_str(), ip.c_str(), port);

    if(deviceConnected_ && !pendingDisconnection_)
    {
        LOGWARN("Unexpected service discovery while already connected - ignoring");
        return;
    }

    if (isIPv4Address(ip))
    {
        LOGINFO("Device service discovered - connecting to device");
        processor_.scheduleTask([this, ip, port] () {
            setDeviceAddress(ip, std::to_string(port));
            setConnectionStatus(false);
            address_ = ip + ":" + std::to_string(port);
            stopMdns();
            connect();
        });
    }
    else
    {
        LOGINFO("Cherry service resolved to IPv6, restarting mDNS in 3s.");
        processor_.scheduleTask([this] () {
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
            restartMdns();
        });
    }
}

void ConnectionManager::onServiceRemove(std::string name, std::string type, std::string domain)
{
    LOGINFO("Service removed name:type:domain %s:%s:%s", name.c_str(), type.c_str(), domain.c_str());
    // Doing nothing because actions should be triggered from websocket closure.
}

// Triggered when browser has cached data but resolving fails -ex.service doesn't exist anymore on local network.
void ConnectionManager::onServiceResolveFailure(std::string name, std::string type, std::string domain)
{
    LOGINFO("Service resolve failure name:type:domain: %s:%s:%s. ", name.c_str(), type.c_str(), domain.c_str());
    if(address_.empty())
    {
        // Service wasn't fully discovered in past.
        LOGINFO("No saved connection data. Waiting for Mdns event.");
    }
    else
    {
        LOGINFO("Closing Mdns and trying to reconnect with last valid uri %s:", address_.c_str());
        processor_.scheduleTask([this] () {
            stopMdns();
            connect();
        });
    }
}

void ConnectionManager::connect()
{
    LOGINFO("Trying to connect web socket client");

    {
        updateUsbInterfaceDetectedState(true);
        if (connectionStatusReason_ != REASON_AUTHENTICATING)
        {
            connectionStatusReason_ = REASON_AUTHENTICATING;
            notifyListener();
        }
    }

    wsClient_.connect(address_, std::bind(&ConnectionManager::onServiceConnection, this, std::placeholders::_1),
        std::bind(&ConnectionManager::onServiceDisconnected, this));
}

void ConnectionManager::onServiceConnection(WebSockets::ConnectionInitializationResult result)
{
    LOGINFO("success: %s", result ? "true" : "false");
    processor_.scheduleTask([this, result] () {
        if (result)
        {
            {
                totalConnections++;
                setConnectionStatus(true);
                pendingDisconnection_ = false; // We have a clean connection, any future disconnection is real
                setDeviceAuthenticatedState(true);
            }
            LOGINFO("Connection successful. Closing Mdns.");
            resetMdns();
        }
        else
        {
            if (!result.authenticationSuccess())
            {
                setDeviceAuthenticatedState(false);
                LOGINFO("Connection NOT successful because of TLS problems. Restarting mDNS discovery.");
                restartMdns();
            }
            else
            {
                LOGINFO("Connection NOT successful. Restarting mDNS discovery");
                restartMdns();
            }
        }
    });
}

void ConnectionManager::onServiceDisconnected()
{
    totalConnections--;
    if((totalConnections > 0) || pendingDisconnection_) // Only retry if the only open connection closed.
    {
        LOGINFO("Service disconnected (WebSocket closed) already connected(count %d) or pending disconnection(%s) -> don't try to reconnect",
                totalConnections, pendingDisconnection_ ? "YES" : "NO");
        return;
    }

    if(!usbInterfaceDetected_)
    {
        LOGINFO("Service disconnected (WebSocket closed). USB not present -> don't try to reconnect");
        return;
    }

    LOGINFO("Service disconnected (WebSocket closed). Restarting mDNS discovery");
    processor_.scheduleTask([this] () {
        if (!interfaceExists(deviceInterface_))
        {
            LOGINFO("USB interface is down");
            updateUsbInterfaceDetectedState(false);
        }
        setConnectionStatus(false);
        restartMdns();
    });
}

void ConnectionManager::setNotificationHandler(std::function<void(const WebSockets::JsonRpc::Notification&)> notificationHandler)
{
    wsClient_.setNotificationHandler(notificationHandler);
}

bool ConnectionManager::rpcCall(const std::string &method, const JsonObject &parameters, JsonObject &res)
{
    // This is just a forward operation, no need to schedule it on processor
    // because it is run by multiple wpeframework threads and we would loose
    // its parallel behaviour
    WebSockets::JsonRpc::Request request;
    WebSockets::JsonRpc::Response response;
    return (request.create(method, parameters) &&
        wsClient_.sendRequest(request, response) &&
        response.getResult(res));
}

bool ConnectionManager::isIPv4Address(const std::string &address) const
{
    struct sockaddr_in sa;
    return (inet_pton(AF_INET, address.c_str(), &(sa.sin_addr)) > 0);
}

void ConnectionManager::setDeviceAddress(const std::string &ipAddress, const std::string &port)
{
    LOGINFO("ipAddress: %s, port: %s", ipAddress.c_str(), port.c_str());
    deviceIPAddress_ = ipAddress;
    devicePort_ = port;
    // No need to notifyListener as connected state must have changed too
}

void ConnectionManager::setConnectionStatus(bool connected)
{
    if (connected != deviceConnected_)
    {
        deviceConnected_ = connected;
        connectionStatusReason_ = deviceConnected_ ? REASON_CONNECTED
            : usbInterfaceDetected_ ? REASON_BOOTING : REASON_DISCONNECTED;
        LOGINFO("deviceConnected: %s", deviceConnected_ ? "true" : "false");
        notifyListener();
    }
}

void ConnectionManager::setDeviceAuthenticatedState(boost::optional<bool> deviceAuthenticated)
{
    if (deviceAuthenticated_ != deviceAuthenticated)
    {
        deviceAuthenticated_ = deviceAuthenticated;
        if (boost::none != deviceAuthenticated_ && false == *deviceAuthenticated_)
        {
            LOGINFO("Setting %s connection status reason.", REASON_INVALID_CERTIFICATE);
            connectionStatusReason_ = REASON_INVALID_CERTIFICATE;
        }
        LOGINFO("Device authentication state: %s", !deviceAuthenticated_ ? "unknown" :
            ((*deviceAuthenticated_) ? "authenticated" : "invalid certifacte"));
        notifyListener();
    }
}

void ConnectionManager::onUsbInterfaceDetectedChange(bool detected)
{
    if(unsettledUsbInterfaceDetected_ == detected) return;

    usbSettlingMutex.lock();

    LOGINFO("onUsbInterfaceDetectedChange: %s WebSocket connected: %s",
            detected ? "DETECTED" : "NOT DETECTED",
            deviceConnected_ ? "YES" : "NO");

    if(!detected) pendingDisconnection_ = true; // At somepoint in the future an existing connection will drop

    unsettledUsbInterfaceDetected_ = detected;

    usbSettlingMutex.unlock();
    usbSettlingCv.notify_one();
}

void ConnectionManager::usbSettlingMonitor()
{
    while(!usbSettlingMonitorExit_)
    {
        std::unique_lock<std::mutex> lock(usbSettlingMutex);

        LOGINFO("Waiting for USB event");
        isSettlingThreadWaiting_ = true;
        usbSettlingCv.wait(lock);

        if(usbSettlingMonitorExit_) break;

        LOGINFO("usbSettlingdMonitor state %s\n", unsettledUsbInterfaceDetected_ ? "DETECTED" : "NOT DETECTED");
        if(unsettledUsbInterfaceDetected_)
        {
            lock.unlock(); // Allow the unsettled state to change whilst we settle
            LOGINFO("USB detected start settling delay");
            connectionStatusReason_ = REASON_BOOTING; // Get the BOOTING status out before waiting.
            notifyListener();
            std::this_thread::sleep_for(std::chrono::milliseconds(USB_SETTLING_TIMEOUT));
            LOGINFO("USB detected settling delay complete");
            lock.lock();

            // We cannot entirely trust udev, so check directly whether the interface exists at the end of settling
            if(deviceInterface_.empty())
            {
                LOGINFO("No interface name set, cannot check for interface, trust udev");
            }
            else if(interfaceExists(deviceInterface_))
            {
                LOGINFO("USB detected after settling delay");
                unsettledUsbInterfaceDetected_ = true;
            }
            else
            {
                LOGINFO("USB not detected after settling delay");
                unsettledUsbInterfaceDetected_ = false;
            }
        }
        onSettledUsbInterfaceDetectedChange(unsettledUsbInterfaceDetected_);
        lock.unlock();
    }
    LOGINFO("USB settling monitor exited");
}

void ConnectionManager::onSettledUsbInterfaceDetectedChange(bool detected)
{
    LOGINFO("OnSettledUsbInterfaceDetected: %s", detected ? "DETECTED" : "NOT DETECTED");

    processor_.scheduleTask([this, detected] () {
        setDeviceAuthenticatedState(boost::none);
        updateUsbInterfaceDetectedState(detected);

        if (detected)
        {
            LOGINFO("Usb interface is up. Restarting mDNS discovery.");
            restartMdns();
        }
        else
        {
            LOGINFO("Stop mDNS discovery if interface has gone away");
            stopMdns();
        }
    });
}

void ConnectionManager::updateUsbInterfaceDetectedState(bool detected)
{
    if (detected != usbInterfaceDetected_)
    {
        usbInterfaceDetected_ = detected;
        connectionStatusReason_ = usbInterfaceDetected_ ? REASON_BOOTING : REASON_DISCONNECTED;
        LOGINFO("usbInterfaceDetected: %s", usbInterfaceDetected_ ? "true" : "false");
        notifyListener();
        setConnectionStatus(false);
    }
}

JsonObject ConnectionManager::serializeStatusToJson() const
{
    JsonObject status;
    status["ipAddress"] = deviceIPAddress_;
    status["port"] = devicePort_;
    status["connected"] = usbInterfaceDetected_ ? isConnected() : false;
    status["usbInterface"] = usbInterfaceDetected_;
    status["reason"] = connectionStatusReason_;
    return status;
}

void ConnectionManager::notifyListener()
{
    if (connectionStatusListener_)
    {
        JsonObject status = serializeStatusToJson();
        if (hasStatusChanged(status)) {
            connectionStatusListener_->onConnectionStatusChanged(status);
            lastStatus_ = status;
        }
    }
}

bool ConnectionManager::hasStatusChanged(const JsonObject &status) const
{
    if (lastStatus_ == boost::none) {
        return true;
    }

    static constexpr std::array<const char *, 5> keys = {
        "ipAddress", "port", "connected", "usbInterface", "reason"
    };

    return std::any_of(keys.cbegin(), keys.cend(),
        [this, status](const char *key) {
            return (*lastStatus_)[key] != status[key];
        });
}

bool ConnectionManager::getConnectionStatus(const JsonObject &parameters, JsonObject &response)
{
    return processor_.request([&response, this] () {
        response = serializeStatusToJson();
        std::string res;
        response.ToString(res);
        LOGINFO("Serialized connection status: %s", res.c_str());
        return true;
    });
}

bool ConnectionManager::isConnected() const
{
    return deviceConnected_.load() && !pendingDisconnection_.load();
}

void ConnectionManager::powerModeChange(const PowerState mode)
{
    LOGINFO("ConnectionManager powerModeChange powerState_=%d mode=%d",powerState_, mode);
    if(mode == powerState_) return;

    if(WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP == powerState_)
    {
        LOGINFO("powerState_ is Deep Sleep in powerModeChange");
        onUsbInterfaceDetectedChange(false);
    }
    powerState_ = mode;
}

std::string ConnectionManager::getInterfaceName()
{
    std::ifstream file(CONFIG_PATH);
    std::string line, interfaceName;

    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key, value;
        if (std::getline(iss, key, '=') && std::getline(iss, value))
        {
            if (key == "CHERRY_INTERFACE")
            {
                interfaceName = value;
                break;
            }
        }
    }
    return interfaceName;
}

bool ConnectionManager::interfaceExists(std::string &ifname)
{
    struct ifaddrs *ifaddr, *ifa;

    getifaddrs(&ifaddr);

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if(ifa->ifa_addr == NULL)
            continue;
        if(strcmp(ifa->ifa_name, ifname.c_str()) == 0)
        {
            freeifaddrs(ifaddr);
            return true;
        }
    }
    freeifaddrs(ifaddr);
    return false;
}

} // namespace Plugin
} // namespace WPEFramework
