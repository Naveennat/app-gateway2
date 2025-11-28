#!/usr/bin/env bash
# Local environment for AppGateway curl tests

export APPGATEWAY_HOST="${APPGATEWAY_HOST:-127.0.0.1}"
export APPGATEWAY_PORT="${APPGATEWAY_PORT:-9998}"
export APPGATEWAY_CALLSIGN="${APPGATEWAY_CALLSIGN:-AppGateway}"
export APPGATEWAY_JSONRPC_URL="${APPGATEWAY_JSONRPC_URL:-http://${APPGATEWAY_HOST}:${APPGATEWAY_PORT}/jsonrpc}"
export CURL_BIN="${CURL_BIN:-curl}"
