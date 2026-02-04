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


Top 18 Apps are successfully launched and all API interactions for Top 18 apps are successful.
This includes Top 18 apps which include Badger APIs

| appName | appId | page | waitTime | appType |
| Apple TV+ | apple_inc_apple_tv | auth | 45 | firebolt |
| Spectrum | charter_spectrum_tv | search | 50 | firebolt |
| DisneyPlus | disneyPlus | auth | 60 | badger |
| Spotify | Spotify | auth | 40 | badger |
| Tubi | Tubi | home | 40 | badger |
| Pluto | pluto | auth | 40 | hybrid |
| Sling | Sling | auth | 40 | badger |
| ESPN | ESPNPlus | home | 40 | badger |
| Hulu | Hulu | auth | 40 | badger |
| Paramount+ | ParamountPlus | auth | 40 | firebolt |
| Xfinity Stream | Comcast_StreamApp | auth | 40 | firebolt |
| Xumo | xumo | other | 40 | firebolt |
| HBO Max | hbo_hbomax | auth | 40 | badger |
| Peacock | Peacock | auth | 40 | hybrid |
| Prime Video | amazonPrime | auth | 40 | hybrid |
| Discovery+ | DiscoveryPlus | auth | 40 | badger |