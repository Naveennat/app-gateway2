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

#include "DeviceBridge.h"
#include "DeviceBridgeUtils.h"
#include "../helpers/utils.h"
#include "core/JSON.h"

#include <strings.h>
#include <vector>
#include <fstream>
#include <arpa/inet.h>
#include "secure_wrapper.h"
#include "UtilsDeviceProperties.h"

// Methods
#define METHOD_DEVICE_BRIDGE_GET_INSTALLED_APPS             "getInstalledAppList"
#define METHOD_DEVICE_BRIDGE_LAUNCH_APP                     "launchApp"
#define METHOD_DEVICE_BRIDGE_CLOSE_APP                      "closeApp"
#define METHOD_DEVICE_BRIDGE_SEND_KEY_EVENT                 "sendKeyEvent"
#define METHOD_DEVICE_BRIDGE_GET_CONNECTION_STATUS          "getConnectionStatus"
#define METHOD_DEVICE_BRIDGE_REBOOT_DEVICE                  "rebootDevice"
#define METHOD_DEVICE_BRIDGE_DEVICE_STANDBY                 "deviceStandby"
#define METHOD_DEVICE_BRIDGE_GET_PROPERTY                   "getProperty"
#define METHOD_DEVICE_BRIDGE_SET_PROPERTY                   "setProperty"
#define METHOD_DEVICE_BRIDGE_CHECK_APP_STATE                "checkAppState"
#define METHOD_DEVICE_BRIDGE_CHANGE_VIEW                    "changeView"
#define METHOD_DEVICE_BRIDGE_GET_FIRMWARE_UPDATE_STATE      "getFirmwareUpdateState"
#define METHOD_DEVICE_BRIDGE_GET_MUTE_CONTROL               "getMuteControl"
#define METHOD_DEVICE_BRIDGE_GET_APP_MUTE_STATE             "getAppMuteState"
#define METHOD_DEVICE_BRIDGE_WIPE_DEVICE_FOR_NEW_PAIRING    "wipeDeviceForNewPairing"
#define METHOD_DEVICE_BRIDGE_FACTORY_RESET_DEVICE           "factoryResetDevice"
#define METHOD_DEVICE_BRIDGE_NOTIFY_HOST_POWER_STATE        "notifyHostPowerState"
#define METHOD_DEVICE_BRIDGE_GET_SOFTWARE_LICENSE           "getSoftwareLicense"
#define METHOD_DEVICE_BRIDGE_TRIGGER_FIRMWARE_CHECK         "triggerFirmwareCheck"
#define METHOD_DEVICE_BRIDGE_SET_APP_MUTE_STATE             "setAppMuteState"
#define METHOD_DEVICE_BRIDGE_SEND_MESSAGE                   "sendMessage"
#define METHOD_DEVICE_BRIDGE_SEND_PA_EVENT                  "sendPAEvent"
#define METHOD_DEVICE_BRIDGE_NOTIFY_MAINTENANCE_MODE        "notifyMaintenanceModeStarted"
#define METHOD_DEVICE_BRIDGE_NOTIFY_FW_USER_CONSENT         "notifyFirmwareUserConsent"

// Events
#define EVENT_DEVICE_BRIDGE_CONNECTION_STATUS_CHANGED       "connectionStatusChanged"

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework {

    namespace {
        static Plugin::Metadata<Plugin::DeviceBridge> metadata(
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

namespace Plugin       {
    DeviceBridge* DeviceBridge::_instance = nullptr;

// returnResponse is a macro and it is not safe to pass function calls to it.
// Something like this `returnResponse(functionThatModifiesResponse(response))`
// will evaluate to this:
// `response["success"] = functionThatModifiesResponse(response);`
// which is susceptible to iterator invalidation

#define prepareResponseAndReturn(status) \
    bool ret = status; \
    returnResponse(ret);

DeviceBridge::DeviceBridge()
    : AbstractPlugin(),
     eventsObserversMutex(), eventsObservers(),
     _registeredEventHandlers(false),
     _pwrMgrNotification(*this)
{
    LOGINFO();

    _instance = this;

    Register(METHOD_DEVICE_BRIDGE_GET_INSTALLED_APPS, &DeviceBridge::getInstalledAppList, this);
    Register(METHOD_DEVICE_BRIDGE_LAUNCH_APP, &DeviceBridge::launchApp, this);
    Register(METHOD_DEVICE_BRIDGE_CLOSE_APP, &DeviceBridge::closeApp, this);
    Register(METHOD_DEVICE_BRIDGE_SEND_KEY_EVENT, &DeviceBridge::sendKeyEvent, this);
    Register(METHOD_DEVICE_BRIDGE_GET_CONNECTION_STATUS, &DeviceBridge::getConnectionStatus, this);
    Register(METHOD_DEVICE_BRIDGE_REBOOT_DEVICE, &DeviceBridge::rebootDevice, this);
    Register(METHOD_DEVICE_BRIDGE_DEVICE_STANDBY, &DeviceBridge::deviceStandby, this);
    Register(METHOD_DEVICE_BRIDGE_GET_PROPERTY, &DeviceBridge::getProperty, this);
    Register(METHOD_DEVICE_BRIDGE_SET_PROPERTY, &DeviceBridge::setProperty, this);
    Register(METHOD_DEVICE_BRIDGE_CHECK_APP_STATE, &DeviceBridge::checkAppState, this);
    Register(METHOD_DEVICE_BRIDGE_CHANGE_VIEW, &DeviceBridge::changeView, this);
    Register(METHOD_DEVICE_BRIDGE_GET_FIRMWARE_UPDATE_STATE, &DeviceBridge::getFirmwareUpdateState, this);
    Register(METHOD_DEVICE_BRIDGE_GET_MUTE_CONTROL, &DeviceBridge::getMuteControl, this);
    Register(METHOD_DEVICE_BRIDGE_GET_APP_MUTE_STATE, &DeviceBridge::getAppMuteState, this);
    Register(METHOD_DEVICE_BRIDGE_WIPE_DEVICE_FOR_NEW_PAIRING, &DeviceBridge::wipeDeviceForNewPairing, this);
    Register(METHOD_DEVICE_BRIDGE_FACTORY_RESET_DEVICE, &DeviceBridge::factoryResetDevice, this);
    Register(METHOD_DEVICE_BRIDGE_NOTIFY_HOST_POWER_STATE, &DeviceBridge::notifyHostPowerState, this);
    Register(METHOD_DEVICE_BRIDGE_GET_SOFTWARE_LICENSE, &DeviceBridge::getSoftwareLicense, this);
    Register(METHOD_DEVICE_BRIDGE_TRIGGER_FIRMWARE_CHECK, &DeviceBridge::triggerFirmwareCheck, this);
    Register(METHOD_DEVICE_BRIDGE_SET_APP_MUTE_STATE, &DeviceBridge::setAppMuteState, this);
    Register(METHOD_DEVICE_BRIDGE_SEND_MESSAGE, &DeviceBridge::sendMessage, this);
    Register(METHOD_DEVICE_BRIDGE_SEND_PA_EVENT, &DeviceBridge::sendPAEvent, this);
    Register(METHOD_DEVICE_BRIDGE_NOTIFY_MAINTENANCE_MODE, &DeviceBridge::notifyMaintenanceModeStarted, this);
    Register(METHOD_DEVICE_BRIDGE_NOTIFY_FW_USER_CONSENT, &DeviceBridge::notifyFirmwareUserConsent, this);
}

DeviceBridge::~DeviceBridge()
{
    LOGINFO();
}

bool DeviceBridge::isDeviceConnected()
{
    return connectionManager.isConnected();
}

void DeviceBridge::RegisterEventsObserver(
    const Exchange::IDeviceBridgeProxy::INotification& sink,
    const std::vector<std::string>& eventNames)
{
    std::lock_guard<std::mutex> lock(eventsObserversMutex);
    for (const auto& name : eventNames)
    {
        auto nameIter = eventsObservers.find(name);
        if (nameIter != eventsObservers.end())
        {
            auto& sinks = nameIter->second;
            auto sinkIter = std::find(sinks.begin(), sinks.end(), &sink);
            if (sinkIter != sinks.end())
            {
                LOGINFO("Already registered as an observer for event: %s", name.c_str());
                continue;
            }
            else
            {
                sinks.push_back(&sink);
            }
        }
        else
        {
            eventsObservers[name] = {&sink};
        }
        sink.AddRef();
        LOGINFO("Adding observer for event: %s", name.c_str());
    }
}

void DeviceBridge::UnregisterEventsObserver(const Exchange::IDeviceBridgeProxy::INotification& sink)
{
    std::lock_guard<std::mutex> lock(eventsObserversMutex);
    for (auto& observer : eventsObservers)
    {
        auto& sinks = observer.second;
        auto sinkIter = std::find(sinks.begin(), sinks.end(), &sink);
        if (sinkIter != sinks.end())
        {
            sinks.erase(sinkIter);
            sink.Release();
            LOGINFO("Removing observer for event: %s", observer.first.c_str());
        }
    }
    purgeEventsObservers();
}

uint32_t DeviceBridge::SendDeviceSpecificCmd(const string& input, string& output)
{
    JsonObject jsonInput(input);
    JsonObject jsonOutput;
    uint32_t retCode = sendDeviceSpecificCmd(jsonInput, jsonOutput);
    jsonOutput.ToString(output);
    return retCode;
}

void DeviceBridge::onConnectionStatusChanged(JsonObject& status)
{
    PowerState pwrStateCurrent = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
    LOGINFO("DeviceBridge::onConnectionStatusChanged");
    if(status["connected"].Boolean())
    {
        pwrStateCurrent = getSystemPowerState();
        LOGINFO("DeviceBridge::pwrStateCurrent: %d", pwrStateCurrent);
        if (WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN != pwrStateCurrent)
        {
            JsonObject parameters;
            JsonObject response;

            // Note: GetPowerState doesn't return the nwStandby setting, but it only matters for DEEP_SLEEP states which we are not in if executing this.
            parameters["mode"] = powerModeEnumToString(pwrStateCurrent, false);
            if(!rpcCall("notifyHostPowerState", parameters, response))
            {
                LOGWARN("Failed to notifyHostPowerState");
            }
        }
        else
        {
                LOGWARN("Failed to getSystemPowerState and is UNKNOWN");
        }
    }
    sendNotify(EVENT_DEVICE_BRIDGE_CONNECTION_STATUS_CHANGED, status);
}

const string DeviceBridge::Initialize(PluginHost::IShell* service)
{
    char deviceName[128];
    if (SearchDeviceName(deviceName, sizeof(deviceName)) != 0 || strcmp(deviceName, "LLAMA") != 0) {
      LOGINFO("DeviceBridge only supported on LLAMA devices\n");
      return std::string("Not supported");
    }
    PowerState pwrStateCurrent = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
    LOGINFO();
    connectionManager.initialize(*this);
    connectionManager.setNotificationHandler(std::bind(&DeviceBridge::onRpcEvent, this, std::placeholders::_1));

    InitializePowerManager(service);
    pwrStateCurrent = getSystemPowerState();
    LOGINFO("DeviceBridge::pwrStateCurrent: %d", pwrStateCurrent);
    if (WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN != pwrStateCurrent) {
        powerModeChange(pwrStateCurrent, false);
    }
    else
    {
        LOGWARN("DeviceBridge::Initialize Failed to getSystemPowerState and is UNKNOWN");
    }
    registerEventHandlers();
    return(string());
}

void DeviceBridge::Deinitialize(PluginHost::IShell* /*service*/)
{
     char deviceName[128];
    if (SearchDeviceName(deviceName, sizeof(deviceName)) != 0 || strcmp(deviceName, "LLAMA") != 0) {
       LOGINFO("DeviceBridge only supported on LLAMA devices\n");
      return;
    }
    LOGINFO("DeviceBridge::Deinitialize\n");
    unregisterEventHandlers();
    DeInitializePowerManager();
    connectionManager.deinitialize();
}

void DeviceBridge::InitializePowerManager(PluginHost::IShell* service)
{
    LOGINFO("DeviceBridge::InitializePowerManager Connect the COM-RPC socket for Device Bridge InitializePowerManager\n");
    _powerManagerPlugin = PowerManagerInterfaceBuilder(_T("org.rdk.PowerManager"))
        .withIShell(service)
        .withRetryIntervalMS(DEVICEBRIDGE_PWRMGR_RETRY_INTERVAL)
        .withRetryCount(DEVICEBRIDGE_PWRMGR_RETRY_COUNT)
        .createInterface();
}

void DeviceBridge::DeInitializePowerManager()
{
    LOGINFO("DeviceBridge::DeInitializePowerManager\n");
    if (_powerManagerPlugin) {
        _powerManagerPlugin.Reset();
    }
}

PowerState DeviceBridge::getSystemPowerState()
{
    PowerState pwrStateCur = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
    PowerState pwrStatePrev = WPEFramework::Exchange::IPowerManager::POWER_STATE_UNKNOWN;
    Core::hresult retStatus = Core::ERROR_GENERAL;

    ASSERT (_powerManagerPlugin);
    if (_powerManagerPlugin){
        retStatus = _powerManagerPlugin->GetPowerState(pwrStateCur, pwrStatePrev);
    }
    if (Core::ERROR_NONE == retStatus)
    {
        LOGINFO("DeviceBridge::getSystemPowerState m_powerState: %d", pwrStateCur);
    }
    else
    {
        LOGWARN("DeviceBridge::getSystemPowerState failed");
    }
    return pwrStateCur;
}

void DeviceBridge::registerEventHandlers()
{
    ASSERT (_powerManagerPlugin);
    LOGINFO("DeviceBridge::registerEventHandlers\n");
    if(!_registeredEventHandlers && _powerManagerPlugin) {
        _powerManagerPlugin->Register(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
        _registeredEventHandlers = true;
    }
}

void DeviceBridge::unregisterEventHandlers()
{
    ASSERT (_powerManagerPlugin);
    LOGINFO("DeviceBridge::unregisterEventHandlers\n");
    if(_registeredEventHandlers && _powerManagerPlugin) {
        _powerManagerPlugin->Unregister(_pwrMgrNotification.baseInterface<Exchange::IPowerManager::IModeChangedNotification>());
        _registeredEventHandlers = false;
    }
}

void DeviceBridge::powerModeChange(const PowerState mode, const bool nwStandByMode)
{
    std::string powerState = powerModeEnumToString(mode, nwStandByMode);
    LOGINFO("DeviceBridge::powerModeChange : %s", powerState.c_str());

    connectionManager.powerModeChange(mode);

    if(isDeviceConnected())
    {
        JsonObject parameters;
        JsonObject response;

        parameters["mode"] = powerModeEnumToString(mode, nwStandByMode);
        if(!rpcCall("notifyHostPowerState", parameters, response))
        {
            LOGWARN("DeviceBridge::Failed to notifyHostPowerState");
        }
    }
    else
    {
        LOGINFO("DeviceBridge::Device not connected, not sending notifyHostPowerState");
    }
}

std::string DeviceBridge::powerModeEnumToString(PowerState state,const bool nwStandbyMode)
{
    std::string powerState = "";
    switch (state)
    {
        case WPEFramework::Exchange::IPowerManager::POWER_STATE_ON: powerState = "ACTIVE"; break;
        case WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF: powerState = nwStandbyMode ? "NETWORKED STANDBY" : "STANDBY"; break;
        case WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY: powerState = "ACTIVE STANDBY"; break;
        case WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_LIGHT_SLEEP: powerState = "ACTIVE STANDBY"; break;
        case WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP: powerState = nwStandbyMode ? "NETWORKED STANDBY" : "STANDBY"; break;
        default: powerState ="UNKNOWN";LOGERR("DeviceBridge::powerState is UnKnown"); break;
    }
    LOGINFO("DeviceBridge::powerModeEnumToString : %s", powerState.c_str());
    return powerState;
}

void DeviceBridge::onPowerModeChanged(const PowerState currentState, const PowerState newState)
{
    LOGINFO("DeviceBridge::onPowerModeChanged Event triggered for PowerStateChange Current State %d, New State: %d",
            currentState,newState);

    if (DeviceBridge::_instance)
    {
        bool nwStandByMode = (currentState == WPEFramework::Exchange::IPowerManager::POWER_STATE_OFF ||
			      currentState == WPEFramework::Exchange::IPowerManager::POWER_STATE_STANDBY_DEEP_SLEEP);
        LOGINFO("DeviceBridge::onPowerModeChanged Invoked powerModeChange nwStandByMode: %s", nwStandByMode ? "true" : "false");
        _instance->powerModeChange(newState, nwStandByMode);
    }
    else
    {
        LOGERR("DeviceBridge::onPowerModeChanged Failed with _instance as NULL");
    }
}

uint32_t DeviceBridge::getInstalledAppList(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get a list of installed apps from device");
        prepareResponseAndReturn(false);
    }
    prepareResponseAndReturn(rpcCall("listApps", parameters, response));
}

uint32_t DeviceBridge::launchApp(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to launch an app on device");
        prepareResponseAndReturn(false);
    }

    returnIfStringParamEmpty(parameters, "name");
    prepareResponseAndReturn(rpcCall("launchApp", parameters, response));
}

uint32_t DeviceBridge::closeApp(const JsonObject &parameters, JsonObject &response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to close an app on device");
        prepareResponseAndReturn(false);
    }

    returnIfStringParamEmpty(parameters, "packageName");
    returnIfBooleanParamNotFound(parameters, "forceClose");
    prepareResponseAndReturn(rpcCall("closeApp", parameters, response));
}

uint32_t DeviceBridge::sendKeyEvent(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to send a Key Event to device");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "keyCode") &&
        validateParameter(parameters, "keyState") &&
        rpcCall("keyEvent", parameters, response));
}

uint32_t DeviceBridge::getConnectionStatus(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();
    prepareResponseAndReturn(connectionManager.getConnectionStatus(parameters, response));
}

uint32_t DeviceBridge::rebootDevice(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to reboot device");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("deviceReboot", parameters, response));
}

uint32_t DeviceBridge::deviceStandby(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to put device into standby");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("deviceStandby", parameters, response));
}

uint32_t DeviceBridge::sendDeviceSpecificCmd(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    const char* keyCmd = "cmd";
    const char* keyParams = "params";
    std::string cmd;
    JsonObject params;

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to send device specific command");
        prepareResponseAndReturn(false);
    }

    // Prepare Rpc Call from parameters: 'cmd' and optional 'params'
    if(validateParameter(parameters, keyCmd) == false)
    {
        LOGERR("Required parameter \"%s\" is not avaliable - unable to send device specific command", keyCmd);
        prepareResponseAndReturn(false);
    }

    getStringParameter(keyCmd, cmd);

    if (parameters.HasLabel(keyParams))
    {
        if(parameters[keyParams].Content() != Core::JSON::Variant::type::OBJECT)
        {
            LOGERR("Optional parameter \"%s\" is not an object - unable to send device specific command", keyParams);
            prepareResponseAndReturn(false);
        }
        params = parameters[keyParams].Object();
    }

    prepareResponseAndReturn(rpcCall(cmd, params, response));
}

uint32_t DeviceBridge::getProperty(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get property from device");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "property") &&
        rpcCall("getProperty", parameters, response));
}

uint32_t DeviceBridge::setProperty(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to set property on device");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "property") &&
        validateParameter(parameters, "value") &&
        rpcCall("setProperty", parameters, response));
}

uint32_t DeviceBridge::checkAppState(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get app state");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "packageName") &&
        rpcCall("checkAppState", parameters, response));
}

uint32_t DeviceBridge::changeView(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to change view");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "appId") &&
        validateParameter(parameters, "viewType") &&
        rpcCall("changeView", parameters, response));
}

uint32_t DeviceBridge::getFirmwareUpdateState(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get firmware update state");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("getFirmwareUpdateState", parameters, response));
}

uint32_t DeviceBridge::getMuteControl(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get a status of mute control");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("getMuteControl", parameters, response));
}

uint32_t DeviceBridge::getAppMuteState(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get an app mute state");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "appId") &&
        rpcCall("getAppMuteState", parameters, response));
}

uint32_t DeviceBridge::wipeDeviceForNewPairing(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to wipe device for new pairing");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("wipeDeviceForNewPairing", parameters, response));
}

uint32_t DeviceBridge::factoryResetDevice(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to factory reset device");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("factoryResetDevice", parameters, response));
}

uint32_t DeviceBridge::notifyHostPowerState(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    LOGWARN("Ignoring external notifyHostPowerState");

    prepareResponseAndReturn(true);
}

uint32_t DeviceBridge::triggerFirmwareCheck(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to trigger firmware check");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("triggerFirmwareCheck", parameters, response));
}

uint32_t DeviceBridge::setAppMuteState(const JsonObject& parameters, JsonObject& response)
{
    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to set app mute state");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "appId") &&
        validateParameter(parameters, "type") &&
        validateParameter(parameters, "value") &&
        rpcCall("setAppMuteState", parameters, response));
}

uint32_t DeviceBridge::sendMessage(const JsonObject& parameters, JsonObject& response)
{
    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to send message");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "appId") &&
        validateParameter(parameters, "message") &&
        rpcCall("sendMessage", parameters, response));
}

uint32_t DeviceBridge::notifyMaintenanceModeStarted(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to notify maintenance mode");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("notifyMaintenanceModeStarted", parameters, response));
}

uint32_t DeviceBridge::notifyFirmwareUserConsent(const JsonObject& parameters, JsonObject& response)
{
    LOGINFOMETHOD();

    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to notify firmware user consent");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(rpcCall("notifyFirmwareUserConsent", parameters, response));
}

bool DeviceBridge::rpcCall(const std::string& method, const JsonObject& parameters, JsonObject& response)
{
    return connectionManager.rpcCall(method, parameters, response);
}

bool DeviceBridge::validateParameter(const JsonObject& parameters, const char* name) const
{
    if (!name || !parameters.HasLabel(name) || parameters[name].String().empty())
    {
        LOGERR("Parameter %s is required.", name);
        return false;
    }
    return true;
}

bool DeviceBridge::delegateToObservers(const std::string& eventName, const std::string& data)
{
    bool delegated = false;
    std::lock_guard<std::mutex> lock(eventsObserversMutex);
    auto nameIter = eventsObservers.find(eventName);
    if (nameIter != eventsObservers.end())
    {
        for (const auto &sink : nameIter->second)
        {
            LOGINFO("Delegating event: %s", eventName.c_str());
            sink->Event(eventName, data);
            delegated = true;
        }
    }
    return delegated;
}

void DeviceBridge::purgeEventsObservers()
{
    auto it = eventsObservers.begin();
    while (it != eventsObservers.end())
    {
        if (it->second.empty())
        {
            LOGINFO("No more observers, removing event: %s", it->first.c_str());
            it = eventsObservers.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

uint32_t DeviceBridge::getSoftwareLicense(const JsonObject& parameters, JsonObject& response)
{
    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to get software license");
        prepareResponseAndReturn(false);
    }

    returnIfNumberParamNotFound(parameters, "offset");
    returnIfNumberParamNotFound(parameters, "length");

    const auto& offset = parameters["offset"].Number();
    if (offset < 0)
    {
        LOGERR("Negative offset provided");
        returnResponse(false);
    }

    const auto& length = parameters["length"].Number();
    if (length < 0)
    {
        LOGERR("Negative length provided");
        returnResponse(false);
    }

    prepareResponseAndReturn(rpcCall("getSoftwareLicense", parameters, response));
}

void DeviceBridge::onRpcEvent(const WebSockets::JsonRpc::Notification& notification)
{
    const std::string method(notification.Designator.Value());
    const std::string params(notification.Parameters.Value());
    JsonObject dataJson(params);

    if (!method.compare("triggerADB"))
    {
        LOGINFO("Handling notification: %s", method.c_str());
        onTriggerADB(dataJson);
    }
    else if (!method.compare("onLaunchAppSubscriptions")) {
        LOGINFO("Handling launchAppSubscriptions");
        onLaunchAppSubscriptions(dataJson);
    }
    else if (!method.compare("onRequestAppToken")) {
        LOGINFO("Handling requestAppToken");
        onRequestAppToken(dataJson);
    }
    else if (!method.compare("onSetAppToken")) {
        LOGINFO("Handling setAppToken");
        onSetAppToken(dataJson);
    }
    else if (!method.compare("onDeleteAppToken")) {
        LOGINFO("Handling deleteAppToken");
        onDeleteAppToken(dataJson);
    }
    else if (!delegateToObservers(method, params))
    {
        // Only send notification if none of the observers were interested
        LOGINFO("Propagating notification: %s", method.c_str());
        sendNotify(method.c_str(), dataJson);
    }
}

void DeviceBridge::onTriggerADB(const JsonObject& parameters)
{
    LOGINFO();

    if (parameters.HasLabel("enable") && parameters["enable"].Content() == Core::JSON::Variant::type::BOOLEAN )
    {
        bool enable = parameters["enable"].Boolean();
        LOGINFO("triggerADB message received: enable: %s", enable ? "true" : "false");

        const std::string enableString = std::string(enable ? "enable" : "disable");
        LOGINFO("Changing ADB settings with command:/lib/rdk/iptables_sky_cherry_dynamic adb %s", enableString.c_str());
        if (v_secure_system("/lib/rdk/iptables_sky_cherry_dynamic adb %s",enableString.c_str()) != 0)
        {
            LOGERR("v_secure_system command failed:/lib/rdk/iptables_sky_cherry_dynamic adb %s", enableString.c_str());
        }
    }
    else
    {
        std::string plainMessage;
        parameters.ToString(plainMessage);
        LOGERR("Malformed triggerADB message: %s", plainMessage.c_str());
    }
}

void DeviceBridge::onLaunchAppSubscriptions(const JsonObject& parameters) {
    LOGINFO();
    if (parameters.HasLabel("appContentData") && parameters["appContentData"].Content() == Core::JSON::Variant::type::STRING)
    {
        std::string appContentData = parameters["appContentData"].String();
        LOGINFO("onLaunchAppSubscriptions message received with appContentData: %s", appContentData.c_str());

        std::string appId = getHostForegroupAppID();
        if (appId.empty())
        {
            LOGERR("No foreground app found!");
            return;
        }
        LOGINFO("Foreground app: %s", appId.c_str());
        if (!checkHostInstalledAppIsCherry(appId))
        {
            LOGERR("Foreground app is not a Cherry app!");
            return;
        }
        LOGINFO("Foreground app is a Cherry app");
        if (!launchAMP(appId, appContentData))
        {
            LOGERR("Failed to launch AMP");
            return;
        }
        LOGINFO("AMP launched");
    }
    else
    {
        std::string plainMessage;
        parameters.ToString(plainMessage);
        LOGERR("Malformed onLaunchAppSubscriptions message: %s", plainMessage.c_str());
    }
}

void DeviceBridge::onRequestAppToken(const JsonObject& parameters) {
    LOGINFO();
    if (parameters.HasLabel("key") && parameters["key"].Content() == Core::JSON::Variant::type::STRING &&
        parameters.HasLabel("scope") && parameters["scope"].Content() == Core::JSON::Variant::type::STRING &&
        parameters.HasLabel("packageName") && parameters["packageName"].Content() == Core::JSON::Variant::type::STRING) 
    {
        std::string key = parameters["key"].String();
        std::string scope = parameters["scope"].String();
        std::string packageName = parameters["packageName"].String();
        LOGINFO("onRequestAppToken message received: key: %s, scope: %s, packageName: %s", key.c_str(), scope.c_str(), packageName.c_str());

        std::string appId = getHostForegroupAppID();
        if (appId.empty())
        {
            LOGERR("No foreground app found!");
            return;
        }
        LOGINFO("Foreground app: %s", appId.c_str());
        if (!checkHostInstalledAppIsCherry(appId))
        {
            LOGERR("Foreground app is not a Cherry app. Notifying with 'success=false'");
            notifyAppTokenGet(false, appId, key, scope, "", packageName);
            return;
        }
        LOGINFO("Foreground app is a Cherry app");
        std::string value = getAppToken(appId, key, scope);
        if (value.empty())
        {
            LOGERR("No app token found. Notifying with 'success=false'");
            notifyAppTokenGet(false, appId, key, scope, "", packageName);
            return;
        }
        LOGINFO("Notifying with token value: %s", value.c_str());
        notifyAppTokenGet(true, appId, key, scope, value, packageName);
    }
    else 
    {
        std::string plainMessage;
        parameters.ToString(plainMessage);
        LOGERR("Malformed onRequestAppToken message: %s", plainMessage.c_str());
    }
}

void DeviceBridge::onSetAppToken(const JsonObject& parameters) {
    LOGINFO();
    if (parameters.HasLabel("key") && parameters["key"].Content() == Core::JSON::Variant::type::STRING &&
        parameters.HasLabel("value") && parameters["value"].Content() == Core::JSON::Variant::type::STRING &&
        parameters.HasLabel("scope") && parameters["scope"].Content() == Core::JSON::Variant::type::STRING &&
        parameters.HasLabel("packageName") && parameters["packageName"].Content() == Core::JSON::Variant::type::STRING) 
    {
        std::string key = parameters["key"].String();
        std::string value = parameters["value"].String();
        std::string scope = parameters["scope"].String();
        std::string packageName = parameters["packageName"].String();
        LOGINFO("onSetAppToken message received: key: %s, value: %s, scope: %s, packageName: %s", key.c_str(), value.c_str(), scope.c_str(), packageName.c_str());

        std::string appId = getHostForegroupAppID();
        if (appId.empty())
        {
            LOGERR("No foreground app found!");
            return;
        }
        LOGINFO("Foreground app: %s", appId.c_str());
        if (!checkHostInstalledAppIsCherry(appId))
        {
            LOGERR("Foreground app is not a Cherry app! Notifying with 'success=false'");
            notifyAppTokenSet(false, key, scope, packageName);
            return;
        }
        if (scope != "device")
        {
            LOGERR("Scope is not 'device'! Notifying with 'success=false'");
            notifyAppTokenSet(false, key, scope, packageName);
            return;
        }
        LOGINFO("Foreground app is a Cherry app and scope is device.");
        if (!setAppToken(appId, key, value, scope))
        {
            LOGERR("Failed to set key value! Notifying with 'success=false'");
            notifyAppTokenSet(false, key, scope, packageName);
            return;
        }
        LOGINFO("Token value set.");
        notifyAppTokenSet(true, key, scope, packageName);
    }
    else 
    {
        std::string plainMessage;
        parameters.ToString(plainMessage);
        LOGERR("Malformed onSetAppToken message: %s", plainMessage.c_str());
    }
}

void DeviceBridge::onDeleteAppToken(const JsonObject& parameters) {
    LOGINFO();
    if (parameters.HasLabel("key") && parameters["key"].Content() == Core::JSON::Variant::type::STRING &&
        parameters.HasLabel("scope") && parameters["scope"].Content() == Core::JSON::Variant::type::STRING)
    {
        std::string key = parameters["key"].String();
        std::string scope = parameters["scope"].String();
        std::string packageName = parameters["packageName"].String();
        LOGINFO("onDeleteAppToken message received: key: %s, scope: %s, packageName: %s", key.c_str(), scope.c_str(), packageName.c_str());

        std::string appId = getHostForegroupAppID();
        if (appId.empty())
        {
            LOGERR("No foreground app found!");
            return;
        }
        LOGINFO("Foreground app: %s", appId.c_str());
        if (!checkHostInstalledAppIsCherry(appId))
        {
            LOGERR("Foreground app is not a Cherry app!");
            return;
        }
        LOGINFO("Foreground app is a Cherry app.");

        if (!deleteAppToken(appId, key, scope))
        {
            LOGERR("Failed to delete key value!");
            return;
        }
        LOGINFO("Token value deleted.");
    }
    else {
        std::string plainMessage;
        parameters.ToString(plainMessage);
        LOGERR("Malformed onDeleteAppToken message: %s", plainMessage.c_str());
    }
}

void DeviceBridge::notifyAppTokenGet(const bool success, const std::string &appID, const std::string &key, const std::string &scope, const std::string &value, const std::string &packageName)
{
    LOGINFO();
    JsonObject parameters;
    JsonObject response;
    parameters.Set("success", success);
    parameters.Set("key", key);
    parameters.Set("scope", scope);
    parameters.Set("packageName", packageName);
    parameters.Set("value", value);

    if (rpcCall("notifyAppToken", parameters, response)) {
        LOGERR("notifyAppToken call failed!");
    } else {
        LOGINFO("notifyAppToken call succeeded.");
    }
}

void DeviceBridge::notifyAppTokenSet(const bool success, const std::string &key, const std::string &scope, const std::string &packageName)
{
    LOGINFO();
    JsonObject parameters;
    JsonObject response;
    parameters.Set("success", success);
    parameters.Set("key", key);
    parameters.Set("scope", scope);
    parameters.Set("packageName", packageName);

    if (rpcCall("setAppTokenResult", parameters, response))
    {
        LOGERR("onSetAppTokenResult call failed!");
    } else {
        LOGINFO("onSetAppTokenResult call succeeded.");
    }
}

uint32_t DeviceBridge::sendPAEvent(const JsonObject& parameters, JsonObject& response)
{
    if (!isDeviceConnected())
    {
        LOGERR("DeviceBridge is not connected - unable to send product analysis data");
        prepareResponseAndReturn(false);
    }

    prepareResponseAndReturn(validateParameter(parameters, "appId") &&
        validateParameter(parameters, "eventName") &&
        validateParameter(parameters, "eventData") &&
        rpcCall("sendPAEvent", parameters, response));
}

#undef prepareResponseAndReturn

} // namespace Plugin
} // namespace WPEFramework
