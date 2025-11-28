# FbDiscovery Thunder Plugin – cURL Test Commands


---

## Activate / Deactivate

```bash
# Activate
curl -d '{
  "jsonrpc":"2.0","id":1,
  "method":"Controller.1.activate",
  "params":{"callsign":"org.rdk.FbDiscovery"}
}' http://127.0.0.1:9998/jsonrpc

# Deactivate
curl -d '{
  "jsonrpc":"2.0","id":2,
  "method":"Controller.1.deactivate",
  "params":{"callsign":"org.rdk.FbDiscovery"}
}' http://127.0.0.1:9998/jsonrpc
```

---

## clearContentAccess

```bash
curl -d '{
  "jsonrpc":"2.0","id":101,
  "method":"org.rdk.FbDiscovery.1.clearContentAccess",
  "params":{
    "context":{"appId":"Peacock"}
  }
}' http://127.0.0.1:9998/jsonrpc
```

---

## contentAccess

```bash
curl -d '{
  "jsonrpc":"2.0","id":102,
  "method":"org.rdk.FbDiscovery.1.contentAccess",
  "params":{
    "context":{"appId":"Peacock"},
    "ids":{
      "availabilities":[
        {"id":"xrn:xvp:entity:series:show-123","available":true}
      ],
      "entitlements":[
        {"id":"xrn:xvp:product:sub:premium","entitled":true}
      ]
    }
  }
}' http://127.0.0.1:9998/jsonrpc
```

---

## signIn

```bash
# With entitlement
curl -d '{
  "jsonrpc":"2.0","id":107,
  "method":"org.rdk.FbDiscovery.1.signIn",
  "params":{
    "context":{"appId":"Peacock"},
    "entitlements":[
      {"productId":"premium-monthly","granted":true}
    ]
  }
}' http://127.0.0.1:9998/jsonrpc

# No entitlements
curl -d '{
  "jsonrpc":"2.0","id":1071,
  "method":"org.rdk.FbDiscovery.1.signIn",
  "params":{
    "context":{"appId":"Peacock"},
    "entitlements":""
  }
}' http://127.0.0.1:9998/jsonrpc

```

---

## watched

```bash
curl -d '{
  "jsonrpc":"2.0","id":110,
  "method":"org.rdk.FbDiscovery.1.watched",
  "params":{
    "context":{"appId":"charter_spectrum_tv"},
    "entityId":"partner.com/entity/123",
    "progress":0.95,
    "completed":true,
    "watchedOn":"2025-09-02T21:15:30Z"
  }
}' http://127.0.0.1:9998/jsonrpc

# Without timestamp
curl -d '{
  "jsonrpc":"2.0","id":1101,
  "method":"org.rdk.FbDiscovery.1.watched",
  "params":{
    "context":{"appId":"charter_spectrum_tv"},
    "entityId":"partner.com/entity/123",
    "progress":0.95,
    "completed":true,
    "watchedOn":""
  }
}' http://127.0.0.1:9998/jsonrpc
```

---

## watchNext

```bash
curl -d '{
  "jsonrpc":"2.0","id":111,
  "method":"org.rdk.FbDiscovery.1.watchNext",
  "params":{
    "context":{"appId":"Peacock"},
    "title":{"default":"Continue watching"},
    "identifiers":{"entityId":"partner.com/entity/123"},
    "expires":"2025-09-05T00:00:00Z",
    "images":{"poster":"https://cdn.example.com/posters/tt1234567.png"}
  }
}' http://127.0.0.1:9998/jsonrpc

```

---

## signOut

```bash
curl -d '{
  "jsonrpc":"2.0","id":108,
  "method":"org.rdk.FbDiscovery.1.signOut",
  "params":{
    "context":{"appId":"Peacock"}
  }
}' http://127.0.0.1:9998/jsonrpc
```


