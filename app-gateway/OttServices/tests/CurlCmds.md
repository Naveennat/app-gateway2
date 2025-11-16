# OttServices Thunder Plugin – cURL Test Commands

---

## Activate / Deactivate

```bash
# Activate
curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.activate",
  "params": { "callsign": "org.rdk.OttServices" }
}' http://127.0.0.1:9998/jsonrpc

# Deactivate

curl -d '{
  "jsonrpc": "2.0",
  "id": 122,
  "method": "Controller.1.deactivate",
  "params": { "callsign": "org.rdk.OttServices" }
}' http://127.0.0.1:9998/jsonrpc
```

----
First call updatePermissionsCache and call getpermissions

----

```bash
curl -s -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","id":4,"method":"org.rdk.OttServices.1.updatePermissionsCache","params":{"appId":"xumo"}}' http://127.0.0.1:9998/jsonrpc

curl -s -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","id":4,"method":"org.rdk.OttServices.1.getPermissions","params":{"appId":"xumo"}}' http://127.0.0.1:9998/jsonrpc
```

To Refresh permissions in cache , call invalidate and then above commands

----

```bash
curl -s -H "Content-Type: application/json" -d '{"jsonrpc":"2.0","id":4,"method":"org.rdk.OttServices.1.invalidatePermissions","params":{"appId":"com.example.app"}}' http://127.0.0.1:9998/jsonrpc
```

----
## Platform Token:

```bash
curl -d '{
  "jsonrpc":"2.0",
  "id":101,
  "method":"org.rdk.OttServices.1.getDistributorToken",
  "params":{ "appId":"comcast.test.firecert" }
}' http://127.0.0.1:9998/jsonrpc
```
