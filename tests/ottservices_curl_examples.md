# OttServices cURL Test Guide

This guide provides example cURL requests to test the OttServices JSON-RPC endpoints exposed by the app gateway.

Assumptions:
- Thunder JSON-RPC endpoint is available at http://127.0.0.1:3001/jsonrpc (adjust host/port as needed)
- Methods:
  - OttServices.1.ott.getDistributorToken
  - OttServices.1.ott.getAuthToken
- Params: { "appId": "<your-app-id>" }

Export some helpers first:
```bash
HOST=127.0.0.1
PORT=3001
JSONRPC=http://$HOST:$PORT/jsonrpc
APP_ID="com.example.app"
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

Notes:
- Successful responses return a JSON-RPC result field with the token string or structured token object depending on implementation.
- If authentication to downstream proto service is required, ensure configuration keys (e.g., TokenEndpoint/UseTlsToken) are set appropriately in the service configuration.
- If `jq` is not installed, remove the final pipe to `jq`.
