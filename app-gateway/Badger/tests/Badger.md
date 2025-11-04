Activate Badger
```bash
curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.activate",
  "params": { "callsign": "org.rdk.Badger" }
}' http://127.0.0.1:9998/jsonrpc
```

Launch Badger
```bash
wscat -c "ws://192.168.1.33:3474?appId=badger"
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "lifecyclemanagement.session",
    "params": {
        "session": {
            "app": {
                "id": "badger",
                "title": "EPG_UI"
            },
            "launch": {
                "inactive": false,
                "intent": {
                    "action": "launch",
                    "context": {
                        "source": "device"
                    }
                }
            },
            "runtime": {
                "id": "uk.sky.runtime.system",
                "transport": "websocket"
            }
        }
    }
}

-> This will give you sessionId
```

Run Badger commands in Badger session

```bash
wscat -c "ws://192.168.1.33:3473?session=<sessionId>"

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.info",
  "params": {appId:"badger"}
}

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.shutdown",
  "params": {"appId": "badger"}
}

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.deviceCapabilities",
  "params": {appId:"badger"}
}

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.networkConnectivity",
  "params": {appId:"badger"}
}

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.getDeviceId",
  "params": {appId:"badger"}
}

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.getDeviceName",
  "params": {appId:"badger"}
}

{
  "jsonrpc": "2.0",
  "id": "110",
  "method": "badger.DismissLoadingScreen",
  "params": {"appId": "badger"}
}
```