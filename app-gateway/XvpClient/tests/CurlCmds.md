
```bash
curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.activate",
  "params": { "callsign": "org.rdk.XvpClient" }
}' http://127.0.0.1:9998/jsonrpc
```

```bash
curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.deactivate",
  "params": { "callsign": "org.rdk.XvpClient" }
}' http://127.0.0.1:9998/jsonrpc
```

