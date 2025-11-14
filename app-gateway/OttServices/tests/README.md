# OttServices Tests

This guide explains how to test the OttServices JSON-RPC methods via cURL. It mirrors the style and intent of FbSettings/tests while focusing on OttServices behavior.

Key JSON-RPC methods:
- OttServices.1.ott.getDistributorToken
- OttServices.1.ott.getAuthToken

Both endpoints accept params: { "appId": "<firebolt-app-id>" } and return a token string (or a structured token object depending on implementation).

## Prerequisites

- OttServices Thunder plugin is loaded/activated.
- JSON-RPC endpoint is reachable. Examples below default to:
  - HOST=127.0.0.1
  - PORT=3001
  - JSON-RPC URL: http://$HOST:$PORT/jsonrpc
- Recommended: jq installed for pretty-printing JSON (optional).

Notes:
- Your deployment may use a different port (e.g., Thunder default 9998 or a customized port). Adjust HOST/PORT accordingly.
- If running against a remote target device, set HOST to its IP/hostname.

## Quick Start

Export variables (adjust as needed):
```bash
export HOST=127.0.0.1
export PORT=3001
export JSONRPC=http://$HOST:$PORT/jsonrpc
export APP_ID="com.example.app"
```

Distributor token:
```bash
curl -sS -X POST "$JSONRPC" \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"OttServices.1.ott.getDistributorToken",
    "params": { "appId": "'"$APP_ID"'" }
  }' | jq .
```

Auth token:
```bash
curl -sS -X POST "$JSONRPC" \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":2,
    "method":"OttServices.1.ott.getAuthToken",
    "params": { "appId": "'"$APP_ID"'" }
  }' | jq .
```

Tip: A helper script and additional examples also exist:
- app-gateway2/tests/ottservices_curl.sh
- app-gateway2/tests/ottservices_curl_examples.md

## Raw JSON Request Examples

getDistributorToken:
```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "method": "OttServices.1.ott.getDistributorToken",
  "params": { "appId": "com.example.app" }
}
```

getAuthToken:
```json
{
  "jsonrpc": "2.0",
  "id": 102,
  "method": "OttServices.1.ott.getAuthToken",
  "params": { "appId": "com.example.app" }
}
```

## Sample Responses

String token (commonly returned):
```json
{
  "jsonrpc": "2.0",
  "id": 101,
  "result": "eyJhbGciOiJIUzI1NiIsInR5cCI6Ikp..."
}
```

Structured token (if implementation wraps additional fields):
```json
{
  "jsonrpc": "2.0",
  "id": 102,
  "result": {
    "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6Ikp...",
    "expiresAt": 1731568200
  }
}
```

Error (invalid params, missing appId):
```json
{
  "jsonrpc": "2.0",
  "id": 103,
  "error": {
    "code": 3,
    "message": "ERROR_BAD_REQUEST"
  }
}
```

Error (method not found, plugin not active or wrong method string):
```json
{
  "jsonrpc": "2.0",
  "id": 104,
  "error": {
    "code": -32601,
    "message": "Method not found"
  }
}
```

## Environment and Host/Port Notes

- HOST: Thunder JSON-RPC host (default: 127.0.0.1).
- PORT: Thunder JSON-RPC port (examples use 3001; your deployment may differ, e.g., 9998).
- APP_ID: Firebolt appId the token request is for (e.g., com.example.app).
- JSONRPC: Constructed as http://$HOST:$PORT/jsonrpc.

Ensure the endpoint is reachable:
```bash
curl -sS -X POST "$JSONRPC" -H 'Content-Type: application/json' -d '{"jsonrpc":"2.0","id":0,"method":"Controller.1.status"}' | jq .
```

## Troubleshooting

1) Plugin initialization / binding
- If OttServices fails to bind to its implementation, it may start in a degraded mode.
- See: app-gateway2/docs/troubleshooting/ottservices-initialization.md for detailed steps.

2) Method not found (-32601)
- The OttServices plugin may not be loaded/activated.
- The method string must be exactly:
  - OttServices.1.ott.getDistributorToken
  - OttServices.1.ott.getAuthToken
- Verify the plugin is active via Thunder Controller and that the version (1) matches your deployment.

3) Bad request / invalid params
- Ensure "params" includes "appId" as a non-empty string.

4) Downstream permission/token service errors
- Validate configuration such as PermissionsEndpoint and UseTls in plugin config.
- Reference: app-gateway2/configs/plugins/OttServices.json.example
- Ensure the device/network can reach the endpoint (firewall/DNS/TLS considerations).

5) Connection refused / no route to host
- HOST or PORT may be incorrect.
- If targeting a remote device, confirm network connectivity and that Thunder is listening on the specified port.

6) Pretty-printing failures
- If jq is not installed, remove the final pipe to jq from the cURL examples.

## References

- Example script: app-gateway2/tests/ottservices_curl.sh
- Example markdown: app-gateway2/tests/ottservices_curl_examples.md
- Troubleshooting: app-gateway2/docs/troubleshooting/ottservices-initialization.md
- Example plugin configuration: app-gateway2/configs/plugins/OttServices.json.example
