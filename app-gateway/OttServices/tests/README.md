# OttServices Thunder Plugin – cURL Test Commands

---

## Activate / Deactivate

```bash
# Activate
curl -d '{
  "jsonrpc":"2.0","id":1,
  "method":"Controller.1.activate",
  "params":{"callsign":"OttServices"}
}' http://$HOST:$PORT/jsonrpc

# Deactivate
curl -d '{
  "jsonrpc":"2.0","id":2,
  "method":"Controller.1.deactivate",
  "params":{"callsign":"OttServices"}
}' http://$HOST:$PORT/jsonrpc
```

---

## Environment

```bash
# Set these to point at your Thunder JSON-RPC endpoint
export HOST=127.0.0.1
export PORT=3001

# Optional helper
export JSONRPC="http://$HOST:$PORT/jsonrpc"
```

Notes:
- If your platform uses a different port (e.g., 9998), adjust PORT accordingly.
- You may invoke using $JSONRPC or http://$HOST:$PORT/jsonrpc interchangeably below.

---

## Method naming rationale

OttServices registers JSON-RPC methods with the names:
- ott.getDistributorToken
- ott.getAuthToken

As per JOttServices registration (see app-gateway2/app-gateway/interfaces/json/JOttServices.h), the effective invocation path is:
- OttServices.1.ott.getDistributorToken
- OttServices.1.ott.getAuthToken

Params:
- { "appId": "<firebolt-app-id>" }

Note: These methods do not accept xACT/SAT parameters. OttServices retrieves tokens internally via Thunder plugins.

---

## ott.getDistributorToken (comcast.test.firecert)

```bash
curl -d '{
  "jsonrpc":"2.0",
  "id":101,
  "method":"OttServices.1.ott.getDistributorToken",
  "params":{ "appId":"comcast.test.firecert" }
}' http://$HOST:$PORT/jsonrpc
```

---

## ott.getDistributorToken (xumo)

```bash
curl -d '{
  "jsonrpc":"2.0",
  "id":102,
  "method":"OttServices.1.ott.getDistributorToken",
  "params":{ "appId":"xumo" }
}' http://$HOST:$PORT/jsonrpc
```

---

## ott.getAuthToken (comcast.test.firecert)

```bash
curl -d '{
  "jsonrpc":"2.0",
  "id":201,
  "method":"OttServices.1.ott.getAuthToken",
  "params":{ "appId":"comcast.test.firecert" }
}' http://$HOST:$PORT/jsonrpc
```

---

## ott.getAuthToken (xumo)

```bash
curl -d '{
  "jsonrpc":"2.0",
  "id":202,
  "method":"OttServices.1.ott.getAuthToken",
  "params":{ "appId":"xumo" }
}' http://$HOST:$PORT/jsonrpc
```

---

## Example expected responses

Success (string token):
```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "result": "eyJhbGciOiJIUzI1NiIsInR5cCI6Ikp..."
}
```

Error (invalid params / missing appId):
```json
{
  "jsonrpc": "2.0",
  "id": 102,
  "error": {
    "code": 3,
    "message": "ERROR_BAD_REQUEST"
  }
}
```

Error (method not found – plugin not active or callsign/version mismatch):
```json
{
  "jsonrpc": "2.0",
  "id": 201,
  "error": {
    "code": -32601,
    "message": "Method not found"
  }
}
```

---

## Troubleshooting

- Ensure the plugin is active:
  - Use the Activate command above or Thunder Controller UI.
- Verify the method path matches your environment:
  - Callsign: OttServices
  - Version: 1
  - Methods: ott.getDistributorToken, ott.getAuthToken
- Confirm host/port:
  - HOST and PORT must match your Thunder JSON-RPC endpoint.
- Check plugin configuration and connectivity:
  - See app-gateway2/configs/plugins/OttServices.json.example
- For initialization/binding issues:
  - See app-gateway2/docs/troubleshooting/ottservices-initialization.md

---

## Related

- Helper script: app-gateway2/tests/ottservices_curl.sh
- Examples: app-gateway2/tests/ottservices_curl_examples.md
- Registration source: app-gateway2/app-gateway/interfaces/json/JOttServices.h
- Example plugin config: app-gateway2/configs/plugins/OttServices.json.example
