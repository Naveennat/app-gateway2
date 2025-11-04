# Launch Delagate JSON-RPC cURL Commands

## Controller: Activate / Deactivate org.rdk.FbMetrics

### Activate
```bash
curl  -d '{"jsonrpc": "2.0","id":122,"method":"Controller.1.activate","params":{"callsign":"org.rdk.LaunchDelegate"}}' http://127.0.0.1:9998/jsonrpc
```

### Deactivate
```bash
curl  -d '{"jsonrpc": "2.0","id":122,"method":"Controller.1.deactivate","params":{"callsign":"org.rdk.LaunchDelegate"}}' http://127.0.0.1:9998/jsonrpc
```

---

## Connect to Launch Delegate port
WS Cat Node tool is used here as reference. Run Launch Delegate using 0.0.0.0:3476 (if Ripple is not running use port 3474).

Enable IPTables on the device using below command

Connect using WSCat output will look something like below
``` bash
~ % wscat -c "ws://<deviceip>:3476?appId=epg"
Connected (press CTRL+C to quit)
```
iptables -I INPUT -p tcp -s <computer ip> --dport <port> -j ACCEPT

### Creating a new session for the app
Once connected use this below command to create a new session
```bash
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "lifecyclemanagement.session",
    "params": {
        "session": {
            "app": {
                "id": "com.xumo.ipa",
                "title": "Xumo Integrated Player"
            },
            "launch": {
                "inactive": true,
                "intent": {
                    "action": "launch",
                    "context": {
                        "source": "device"
                    }
                }
            },
            "runtime": {
                "id": "com.sky.rdkbrowser",
                "transport": "websocket"
            }
        }
    }
}
```

Expected Output
```bash
{"jsonrpc":"2.0","id":1,"result":{"appId":"com.xumo.ipa","sessionId":"5ee3a2c4-1e26-4300-bd51-5f20a920feee","loadedSessionId":"0747b15b-4147-439b-84fb-795b4e8270d5","transitionPending":false}}
```

Request for Charter Spectrum App
```bash
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "lifecyclemanagement.session",
    "params": {
        "session": {
            "app": {
                "id": "charter_spectrum_tv",
                "title": "Charter Spectrum",
                "catalog": "charter_spectrum_tv"
            },
            "launch": {
                "inactive": true,
                "intent": {
                    "action": "launch",
                    "context": {
                        "source": "device"
                    }
                }
            },
            "runtime": {
                "id": "com.sky.rdkbrowser",
                "transport": "websocket"
            }
        }
    }
}
```

---

### Validating the session

You can use the Thunder port to validate this session
```bash
curl -H 'Content-Type: application/json' -d '{
  "jsonrpc":"2.0","id":"100",
  "method":"org.rdk.LaunchDelegate.Authenticate",
  "params": {
    "sessionId": "5ee3a2c4-1e26-4300-bd51-5f20a920feee"
  }
}' http://127.0.0.1:9998/jsonrpc
```

Expected Output
```bash
{"jsonrpc":"2.0","id":1,"result":"com.xumo.ipa"}
```

### getSessionId

```bash
curl -H 'Content-Type: application/json' -d '{
  "jsonrpc":"2.0","id":"101",
  "method":"org.rdk.LaunchDelegate.getSessionId",
  "params": {
    "appId": "charter_spectrum_tv"
  }
}' http://127.0.0.1:9998/jsonrpc
```

### getContentPartnerId

```bash
curl -H 'Content-Type: application/json' -d '{
  "jsonrpc":"2.0","id":"101",
  "method":"org.rdk.LaunchDelegate.getContentPartnerId",
  "params": {
    "appId": "charter_spectrum_tv"
  }
}' http://127.0.0.1:9998/jsonrpc
```

