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

#include "Module.h"
#include "utils.h"
#include "AbstractPlugin.h"

#include <mutex>
#include <atomic>
#include <algorithm>
#include <set>

#include "ConnectionManager.h"
#include "IDeviceBridgeProxy.h"
#include "IConnectionStatusListener.h"


#include "WebSockets/JsonRpc/Notification.h"
#include <interfaces/IPowerManager.h>
#include "PowerManagerInterface.h"


using namespace WPEFramework::Exchange;
using PowerState = WPEFramework::Exchange::IPowerManager::PowerState;

namespace WPEFramework {
namespace Plugin       {

// This is a server for a JSONRPC communication channel.
// For a plugin to be capable to handle JSONRPC, inherit from PluginHost::JSONRPC.
// By inheriting from this class, the plugin realizes the interface PluginHost::IDispatcher.
// This realization of this interface implements, by default, the following methods on this plugin
// - exists
// - register
// - unregister
// Any other methood to be handled by this plugin  can be added can be added by using the
// templated methods Register on the PluginHost::JSONRPC class.
// As the registration/unregistration of notifications is realized by the class PluginHost::JSONRPC,
// this class exposes a public method called, Notify(), using this methods, all subscribed clients
// will receive a JSONRPC message as a notification, in case this method is called.

class DeviceBridge : public AbstractPlugin, public Exchange::IDeviceBridgeProxy, public IConnectionStatusListener
{
public:
    DeviceBridge();
    virtual ~DeviceBridge();

    BEGIN_INTERFACE_MAP(DeviceBridge)
    INTERFACE_ENTRY(Exchange::IDeviceBridgeProxy)
    NEXT_INTERFACE_MAP(AbstractPlugin)

    // Inherited from Exchange::IDeviceBridgeProxy
    virtual void RegisterEventsObserver(
        const Exchange::IDeviceBridgeProxy::INotification& sink,
        const std::vector<std::string>& eventNames) override;
    virtual void UnregisterEventsObserver(const Exchange::IDeviceBridgeProxy::INotification& sink) override;
    virtual uint32_t SendDeviceSpecificCmd(const string& input, string& output) override;

    // Inherited from IConnectionStatusListener
    virtual void onConnectionStatusChanged(JsonObject& status) override;

private:

    class PowerManagerNotification : public Exchange::IPowerManager::IModeChangedNotification {
            private:
                PowerManagerNotification(const PowerManagerNotification&) = delete;
                PowerManagerNotification& operator=(const PowerManagerNotification&) = delete;

            public:
                explicit PowerManagerNotification(DeviceBridge& parent)
                    : _parent(parent)
                {
                }
                ~PowerManagerNotification() override = default;

            public:
                void OnPowerModeChanged(const PowerState currentState, const PowerState newState) override
                {
                    _parent.onPowerModeChanged(currentState, newState);
                }

                template <typename T>
                T* baseInterface()
                {
                    static_assert(std::is_base_of<T, PowerManagerNotification>(), "base type mismatch");
                    return static_cast<T*>(this);
                }

                BEGIN_INTERFACE_MAP(PowerManagerNotification)
                INTERFACE_ENTRY(Exchange::IPowerManager::IModeChangedNotification)
                END_INTERFACE_MAP

            private:
                DeviceBridge& _parent;
    };

    // We do not allow this plugin to be copied !!
    DeviceBridge(const DeviceBridge&) = delete;
    DeviceBridge &operator=(const DeviceBridge&) = delete;
    static DeviceBridge* _instance;

    virtual const string Initialize(PluginHost::IShell* service) override;
    virtual void Deinitialize(PluginHost::IShell* service) override;


    //Begin methods
    uint32_t getInstalledAppList(const JsonObject& parameters, JsonObject& response);
    uint32_t launchApp(const JsonObject& parameters, JsonObject& response);
    uint32_t closeApp(const JsonObject& parameters, JsonObject& response);
    uint32_t sendKeyEvent(const JsonObject& parameters, JsonObject& response);
    uint32_t getConnectionStatus(const JsonObject& parameters, JsonObject& response);
    uint32_t rebootDevice(const JsonObject& parameters, JsonObject& response);
    uint32_t deviceStandby(const JsonObject& parameters, JsonObject& response);
    uint32_t sendDeviceSpecificCmd(const JsonObject& parameters, JsonObject& response);
    uint32_t getProperty(const JsonObject& parameters, JsonObject& response);
    uint32_t setProperty(const JsonObject& parameters, JsonObject& response);
    uint32_t checkAppState(const JsonObject& parameters, JsonObject& response);
    uint32_t changeView(const JsonObject& parameters, JsonObject& response);
    uint32_t getFirmwareUpdateState(const JsonObject& parameters, JsonObject& response);
    uint32_t getMuteControl(const JsonObject& parameters, JsonObject& response);
    uint32_t getAppMuteState(const JsonObject& parameters, JsonObject& response);
    uint32_t wipeDeviceForNewPairing(const JsonObject& parameters, JsonObject& response);
    uint32_t factoryResetDevice(const JsonObject& parameters, JsonObject& response);
    uint32_t notifyHostPowerState(const JsonObject& parameters, JsonObject& response);
    uint32_t getSoftwareLicense(const JsonObject& parameters, JsonObject& response);
    uint32_t triggerFirmwareCheck(const JsonObject& parameters, JsonObject& response);
    uint32_t setAppMuteState(const JsonObject& parameters, JsonObject& response);
    uint32_t sendMessage(const JsonObject& parameters, JsonObject& response);
    uint32_t sendPAEvent(const JsonObject& parameters, JsonObject& response);
    uint32_t notifyMaintenanceModeStarted(const JsonObject& parameters, JsonObject& response);
    uint32_t notifyFirmwareUserConsent(const JsonObject& parameters, JsonObject& response);
    //End methods


    // helper methods
    bool validateParameter(const JsonObject& parameters, const char* name) const;
    bool rpcCall(const std::string& method, const JsonObject& parameters, JsonObject& response);
    bool isDeviceConnected();
    bool delegateToObservers(const std::string& eventName, const std::string& data);
    void purgeEventsObservers();
    void onRpcEvent(const WebSockets::JsonRpc::Notification&);
    void onTriggerADB(const JsonObject& parameters);
    void onLaunchAppSubscriptions(const JsonObject& parameters);
    void onRequestAppToken(const JsonObject& parameters);
    void onSetAppToken(const JsonObject& parameters);
    void onDeleteAppToken(const JsonObject& parameters);
    void notifyAppTokenGet(const bool success, const std::string &appID, const std::string &key, const std::string &scope, const std::string &value, const std::string &packageName);
    void notifyAppTokenSet(const bool success, const std::string &key, const std::string &scope, const std::string &packageName);
    void powerModeChange(const PowerState mode, const bool nwStandByMode);
    std::string powerModeEnumToString(PowerState state,const bool nwStandbyMode);
    void onPowerModeChanged(const PowerState currentState, const PowerState newState);
    PowerState getSystemPowerState();
    void registerEventHandlers();
    void unregisterEventHandlers();
    void InitializePowerManager(PluginHost::IShell* service);
    void DeInitializePowerManager();
    // member variables
    ConnectionManager connectionManager;
    std::mutex eventsObserversMutex;
    std::map<std::string, std::vector<const Exchange::IDeviceBridgeProxy::INotification*>> eventsObservers;

    //power manager variables
    PowerManagerInterfaceRef _powerManagerPlugin;
    bool _registeredEventHandlers;
    Core::Sink<PowerManagerNotification> _pwrMgrNotification;
};

} // namespace Plugin
} // namespace WPEFramework
