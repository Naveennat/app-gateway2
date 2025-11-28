Use LaunchDelegate port to create a session
Make sure below commands are executed
iptables -I INPUT -p tcp -s <deviceip> --dport 3476 -j ACCEPT
iptables -I INPUT -p tcp -s <deviceip> --dport 3475 -j ACCEPT

We use wscat for these below examples but any websocket client can be used.

wscat -c "ws://10.0.0.208:3475?appId=com.bskyb.epgui"

```bash
{
    "jsonrpc": "2.0",
    "id": 1,
    "method": "lifecyclemanagement.session",
    "params": {
        "session": {
            "app": {
                "id": "com.bskyb.epgui",
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
```

Response will look something like below
```
{"jsonrpc":"2.0","id":1,"result":{"appId":"com.bskyb.epgui","sessionId":"0b92b093-7452-4262-8954-8e3f8453c394","loadedSessionId":"be45c9f2-a6d3-469e-907f-a0b2fc6d0200","activeSessionId":"43aa3cab-d132-4098-917e-e16dbf04f445","transitionPending":false}}
```

After getting the session value set websocket connection to AppGateway port (3476)

Below command must be executed
```
{"jsonrpc":"2.0","id":1,"method":"Advertising.advertisingId","params":{"options":{"scope":{"id":"paidPlacement","type":"browse"}}}}
```