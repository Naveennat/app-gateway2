#!/usr/bin/env bash
set -euo pipefail

HOST="${HOST:-127.0.0.1}"
PORT="${PORT:-3001}"
APP_ID="${APP_ID:-com.example.app}"
JSONRPC="http://$HOST:$PORT/jsonrpc"

echo "Using JSON-RPC endpoint: $JSONRPC"
echo "Using APP_ID: $APP_ID"

echo
echo "== Distributor Token =="
curl -sS -X POST "$JSONRPC" \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"OttServices.1.ott.getDistributorToken",
    "params": { "appId": "'"$APP_ID"'" }
  }' | jq . || true

echo
echo "== Auth Token =="
curl -sS -X POST "$JSONRPC" \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":2,
    "method":"OttServices.1.ott.getAuthToken",
    "params": { "appId": "'"$APP_ID"'" }
  }' | jq . || true

# Usage:
#   chmod +x tests/ottservices_curl.sh
#   HOST=127.0.0.1 PORT=3001 APP_ID=com.example.app ./tests/ottservices_curl.sh
