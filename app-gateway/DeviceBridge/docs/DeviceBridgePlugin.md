<!-- Generated automatically, DO NOT EDIT! -->
<a name="head.DeviceBridge_Plugin"></a>
# DeviceBridge Plugin

**Version: 1.0**

**Status: :black_circle::black_circle::black_circle:**

A org.rdk.DeviceBridge plugin for Thunder framework.

### Table of Contents

- [Introduction](#head.Introduction)
- [Description](#head.Description)
- [Configuration](#head.Configuration)
- [Methods](#head.Methods)
- [Notifications](#head.Notifications)

<a name="head.Introduction"></a>
# Introduction

<a name="head.Scope"></a>
## Scope

This document describes purpose and functionality of the org.rdk.DeviceBridge plugin. It includes detailed specification about its configuration, methods provided and notifications sent.

<a name="head.Case_Sensitivity"></a>
## Case Sensitivity

All identifiers of the interfaces described in this document are case-sensitive. Thus, unless stated otherwise, all keywords, entities, properties, relations and actions should be treated as such.

<a name="head.Acronyms,_Abbreviations_and_Terms"></a>
## Acronyms, Abbreviations and Terms

The table below provides and overview of acronyms used in this document and their definitions.

| Acronym | Description |
| :-------- | :-------- |
| <a name="acronym.API">API</a> | Application Programming Interface |
| <a name="acronym.HTTP">HTTP</a> | Hypertext Transfer Protocol |
| <a name="acronym.JSON">JSON</a> | JavaScript Object Notation; a data interchange format |
| <a name="acronym.JSON-RPC">JSON-RPC</a> | A remote procedure call protocol encoded in JSON |

The table below provides and overview of terms and abbreviations used in this document and their definitions.

| Term | Description |
| :-------- | :-------- |
| <a name="term.callsign">callsign</a> | The name given to an instance of a plugin. One plugin can be instantiated multiple times, but each instance the instance name, callsign, must be unique. |

<a name="head.References"></a>
## References

| Ref ID | Description |
| :-------- | :-------- |
| <a name="ref.HTTP">[HTTP](http://www.w3.org/Protocols)</a> | HTTP specification |
| <a name="ref.JSON-RPC">[JSON-RPC](https://www.jsonrpc.org/specification)</a> | JSON-RPC 2.0 specification |
| <a name="ref.JSON">[JSON](http://www.json.org/)</a> | JSON specification |
| <a name="ref.Thunder">[Thunder](https://github.com/WebPlatformForEmbedded/Thunder/blob/master/doc/WPE%20-%20API%20-%20WPEFramework.docx)</a> | Thunder API Reference |

<a name="head.Description"></a>
# Description

The `DeviceBridge` plugin allows you to interact with an accessory that is connected to an RDK device over a bi-directional communication channel.

The plugin is designed to be loaded and executed within the Thunder framework. For more information about the framework refer to [[Thunder](#ref.Thunder)].

<a name="head.Configuration"></a>
# Configuration

The table below lists configuration options of the plugin.

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| callsign | string | Plugin instance name (default: *org.rdk.DeviceBridge*) |
| classname | string | Class name: *org.rdk.DeviceBridge* |
| locator | string | Library name: *libWPEFrameworkDeviceBridge.so* |
| autostart | boolean | Determines if the plugin shall be started automatically along with the framework |

<a name="head.Methods"></a>
# Methods

The following methods are provided by the org.rdk.DeviceBridge plugin:

DeviceBridge interface methods:

| Method | Description |
| :-------- | :-------- |
| [changeView](#method.changeView) | Changes the configured view layout on the connected accessory or informs the accessory about a layout change on the display |
| [checkAppState](#method.checkAppState) | Returns the state of the specified application |
| [closeApp](#method.closeApp) | Closes an application or moves the application to the background on a connected accessory |
| [deviceStandby](#method.deviceStandby) | Puts the connected accessory into standby |
| [factoryResetDevice](#method.factoryResetDevice) | Performs a factory reset of the connected accessory (wipes the user data and reboots) |
| [getAppMuteState](#method.getAppMuteState) | Gets the current application mute state for the microphone and camera |
| [getConnectionStatus](#method.getConnectionStatus) | Returns the connection status of an accessory |
| [getFirmwareUpdateState](#method.getFirmwareUpdateState) | Returns the firmware update state from the connected accessory |
| [getInstalledAppList](#method.getInstalledAppList) | Lists the applications installed on a connected accessory |
| [getMuteControl](#method.getMuteControl) | Gets the current accessory mute state for the microphone and camera |
| [getProperty](#method.getProperty) | Gets a property on the connected accessory |
| [getSoftwareLicense](#method.getSoftwareLicense) | Retrieves a license string from the connected accessory |
| [launchApp](#method.launchApp) | Launches an application on a connected accessory |
| [notifyHostPowerState](#method.notifyHostPowerState) | Notifies the connected accessory about the power state of the device |
| [rebootDevice](#method.rebootDevice) | Reboots the connected accessory |
| [sendKeyEvent](#method.sendKeyEvent) | Sends a remote control key press to a connected accessory |
| [setAppMuteState](#method.setAppMuteState) | Sets the application mute state for the camera or microphone on the connected accessory |
| [setProperty](#method.setProperty) | Sets a property on the connected accessory |
| [triggerFirmwareInstall](#method.triggerFirmwareInstall) | Triggers a firmware install on the connected accessory |
| [wipeDeviceForNewPairing](#method.wipeDeviceForNewPairing) | Sets all the host and household values on the connected accessory to their defaults without performing an FSR or AFR |


<a name="method.changeView"></a>
## *changeView [<sup>method</sup>](#head.Methods)*

Changes the configured view layout on the connected accessory or informs the accessory about a layout change on the display.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.appId | string | The application package name |
| params.viewType | string | The view layout (must be one of the following: *FULLSCREEN*, *PIP*, *SIDEBAR_HORIZONTAL*, *SIDEBAR_VERTICAL*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.changeView",
    "params": {
        "appId": "us.zoom.videomeetings",
        "viewType": "FULLSCREEN"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.checkAppState"></a>
## *checkAppState [<sup>method</sup>](#head.Methods)*

Returns the state of the specified application.
 
**States** 
| State | Description | 
| :----------- | :----------- | 
| `READY` | The application is installed but not launched | 
| `LAUNCHING` | The application is created but not yet in the Foreground | 
| `FOREGROUND` | The application is in the foreground | 
| `BACKGROUND` | The application is in the background | 
| `CLOSING` | The application is no longer visible to the user | 
| `TERMINATED` | The application is destroyed by the system | 
| `UNKNOWN` | The application is not installed on the device |.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.packageName | string | The application package name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.packageName | string | The application package name |
| result.state | string | The current state of the application |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.checkAppState",
    "params": {
        "packageName": "us.zoom.videomeetings"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "packageName": "us.zoom.videomeetings",
        "state": "FOREGROUND",
        "success": true
    }
}
```

<a name="method.closeApp"></a>
## *closeApp [<sup>method</sup>](#head.Methods)*

Closes an application or moves the application to the background on a connected accessory. The call is considered successful if the application reaches the desired state even if no action was completed. When an application goes to the background, the launcher comes to the foreground.  
 
**Outcomes** 
| Precondition | forceClose value | Description |  
| :----------- | :----------- | :----------- | 
| Application in foreground | false | Application moves to background | 
| Application in foreground | true | Application closes | 
| Application in background | false | Nothing happens | 
| Application in background | true | Application closes | 
| Application not started/installed | false | Nothing happens | 
| Application not started/installed | true | Nothing happens |.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.packageName | string | The application package name |
| params.forceClose | boolean | Whether to close the application (`true`) or move the application to the background (`false`) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.closeApp",
    "params": {
        "packageName": "us.zoom.videomeetings",
        "forceClose": true
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.deviceStandby"></a>
## *deviceStandby [<sup>method</sup>](#head.Methods)*

Puts the connected accessory into standby.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.deviceStandby"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.factoryResetDevice"></a>
## *factoryResetDevice [<sup>method</sup>](#head.Methods)*

Performs a factory reset of the connected accessory (wipes the user data and reboots).

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.factoryResetDevice"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.getAppMuteState"></a>
## *getAppMuteState [<sup>method</sup>](#head.Methods)*

Gets the current application mute state for the microphone and camera.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.appId | string | The application package name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.camera | boolean | Whether the camera is muted |
| result.mic | boolean | Whether the mic is muted |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getAppMuteState",
    "params": {
        "appId": "us.zoom.videomeetings"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "camera": true,
        "mic": true,
        "success": true
    }
}
```

<a name="method.getConnectionStatus"></a>
## *getConnectionStatus [<sup>method</sup>](#head.Methods)*

Returns the connection status of an accessory.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.ipAddress | string | The IP address of the accessory |
| result.port | string | The port of the accessory |
| result.connected | boolean | `true` if the accessory is currently connected, otherwise `false` |
| result.usbInterface | boolean | `true` if the USB interface is detected, otherwise `false` |
| result.reason | string | the connection status (must be one of the following: *BOOTING*, *AUTHENTICATING*, *INVALID_CERTIFICATE*, *CONNECTED*, *DISCONNECTED*) |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getConnectionStatus"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "ipAddress": "192.168.0.84",
        "port": "45678",
        "connected": true,
        "usbInterface": true,
        "reason": "CONNECTED",
        "success": true
    }
}
```

<a name="method.getFirmwareUpdateState"></a>
## *getFirmwareUpdateState [<sup>method</sup>](#head.Methods)*

Returns the firmware update state from the connected accessory.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.state | string | The firmware update state |
| result.compulsory | boolean | `true` if the firmware update is compulsory, otherwise `false` |
| result.progress | integer | The percent progress of the entire flow |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getFirmwareUpdateState"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "state": "NO_UPDATE",
        "compulsory": false,
        "progress": 0,
        "success": true
    }
}
```

<a name="method.getInstalledAppList"></a>
## *getInstalledAppList [<sup>method</sup>](#head.Methods)*

Lists the applications installed on a connected accessory. If a package name is included as part of the request, then only information about the specific application is returned. If no package name is included, then information about all installed applications is returned. 
 
### Events
 
 No Events.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params?.packageName | string | <sup>*(optional)*</sup> The application package name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.installedApps | array | Information about installed application(s) |
| result.installedApps[#] | object |  |
| result.installedApps[#].packageName | string | The application package name |
| result.installedApps[#].version | string | The application version |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getInstalledAppList",
    "params": {
        "packageName": "us.zoom.videomeetings"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "installedApps": [
            {
                "packageName": "us.zoom.videomeetings",
                "version": "1.0.0.61"
            }
        ],
        "success": true
    }
}
```

<a name="method.getMuteControl"></a>
## *getMuteControl [<sup>method</sup>](#head.Methods)*

Gets the current accessory mute state for the microphone and camera.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.muted | boolean | Whether the microphone and camera are muted |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getMuteControl"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "muted": true,
        "success": true
    }
}
```

<a name="method.getProperty"></a>
## *getProperty [<sup>method</sup>](#head.Methods)*

Gets a property on the connected accessory.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.property | string | The property name |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result?.deviceSerial | string | <sup>*(optional)*</sup> The property name and value |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getProperty",
    "params": {
        "property": "deviceSerial"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "deviceSerial": "VC02SK0213100056N",
        "success": true
    }
}
```

<a name="method.getSoftwareLicense"></a>
## *getSoftwareLicense [<sup>method</sup>](#head.Methods)*

Retrieves a license string from the connected accessory.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.licenseText | string | The license |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.getSoftwareLicense"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "licenseText": "...",
        "success": true
    }
}
```

<a name="method.launchApp"></a>
## *launchApp [<sup>method</sup>](#head.Methods)*

Launches an application on a connected accessory.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.name | string | The application package name |
| params?.dataUri | string | <sup>*(optional)*</sup> A data URI that is passed to the application |
| params?.componentName | string | <sup>*(optional)*</sup> The exact application component that should receive the dat URI when `dataUri` is defined. The value can be omitted even when a data URI is defined |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.launchApp",
    "params": {
        "name": "us.zoom.videomeetings",
        "dataUri": "zoomus://zoom.us/join?confno=<room number>&pwd=<room password>&zc=0&uname=Betty",
        "componentName": "us.zoom.videomeetings/com.zipow.videobox.JoinByURLActivity"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.notifyHostPowerState"></a>
## *notifyHostPowerState [<sup>method</sup>](#head.Methods)*

Notifies the connected accessory about the power state of the device.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.mode | string | The power state (must be one of the following: *ACTIVE*, *ACTIVE STANDBY*, *NETWORKED STANDBY*, *STANDBY*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.notifyHostPowerState",
    "params": {
        "mode": "STANDBY"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.rebootDevice"></a>
## *rebootDevice [<sup>method</sup>](#head.Methods)*

Reboots the connected accessory.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.rebootDevice"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.sendKeyEvent"></a>
## *sendKeyEvent [<sup>method</sup>](#head.Methods)*

Sends a remote control key press to a connected accessory.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.keyCode | string | An RDK-native RCU key code |
| params.keyState | string | The key state. Either down (`0`) or up (`1`). (must be one of the following: *0*, *1*) |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.sendKeyEvent",
    "params": {
        "keyCode": "39",
        "keyState": "0"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.setAppMuteState"></a>
## *setAppMuteState [<sup>method</sup>](#head.Methods)*

Sets the application mute state for the camera or microphone on the connected accessory.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.appId | string | The application package name |
| params.type | string | The camera or the microphone component (must be one of the following: *Camera*, *Mic*) |
| params.value | boolean | The mute state. `true` for muted or `false` for unmuted |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.setAppMuteState",
    "params": {
        "appId": "us.zoom.videomeetings",
        "type": "Mic",
        "value": true
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.setProperty"></a>
## *setProperty [<sup>method</sup>](#head.Methods)*

Sets a property on the connected accessory.  

 **Supported Properties** 
* `btMAC`  
* `deviceSerial` 
* `deviceModelName` 
* `deviceMAC` 
* `firmwareVersion` 
* `altFirmwareVersion` 
* `ro.vendor.build.sky.release.version` 
* `pairedDeviceMAC` 
* `pairedDeviceSerial` 
* `pairedFirmwareVersion` 
* `pairedDeviceAccountID` 
* `pairedBluetoothMAC` 
* `pairedDeviceLocation`.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.property | string | The property name |
| params.value | string | The property value |

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.setProperty",
    "params": {
        "property": "deviceSerial",
        "value": "VC02SK0213100056N"
    }
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.triggerFirmwareInstall"></a>
## *triggerFirmwareInstall [<sup>method</sup>](#head.Methods)*

Triggers a firmware install on the connected accessory. 

 **NOTE**: This method currently calls `triggerFirmwareCheck` due to unimplemented device manager functions.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.triggerFirmwareInstall"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="method.wipeDeviceForNewPairing"></a>
## *wipeDeviceForNewPairing [<sup>method</sup>](#head.Methods)*

Sets all the host and household values on the connected accessory to their defaults without performing an FSR or AFR.

### Parameters

This method takes no parameters.

### Result

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| result | object |  |
| result.success | boolean | Whether the request succeeded |

### Example

#### Request

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "method": "org.rdk.DeviceBridge.1.wipeDeviceForNewPairing"
}
```

#### Response

```json
{
    "jsonrpc": "2.0",
    "id": 42,
    "result": {
        "success": true
    }
}
```

<a name="head.Notifications"></a>
# Notifications

Notifications are autonomous events, triggered by the internals of the implementation, and broadcasted via JSON-RPC to all registered observers. Refer to [[Thunder](#ref.Thunder)] for information on how to register for a notification.

The following events are provided by the org.rdk.DeviceBridge plugin:

DeviceBridge interface events:

| Event | Description |
| :-------- | :-------- |
| [ConnectionStatusChanged](#event.ConnectionStatusChanged) | Triggered when the status of the connection to the accessory changes |


<a name="event.ConnectionStatusChanged"></a>
## *ConnectionStatusChanged [<sup>event</sup>](#head.Notifications)*

Triggered when the status of the connection to the accessory changes.

### Parameters

| Name | Type | Description |
| :-------- | :-------- | :-------- |
| params | object |  |
| params.ipAddress | string | The IP address of the accessory |
| params.port | string | The port of the accessory |
| params.connected | boolean | `true` if the accessory is currently connected, otherwise `false` |
| params.usbInterface | boolean | `true` if the USB interface is detected, otherwise `false` |
| params.reason | string | the connection status (must be one of the following: *BOOTING*, *AUTHENTICATING*, *INVALID_CERTIFICATE*, *CONNECTED*, *DISCONNECTED*) |
| params.success | boolean | Whether the request succeeded |

### Example

```json
{
    "jsonrpc": "2.0",
    "method": "client.events.1.ConnectionStatusChanged",
    "params": {
        "ipAddress": "192.168.0.84",
        "port": "45678",
        "connected": true,
        "usbInterface": true,
        "reason": "CONNECTED",
        "success": true
    }
}
```

