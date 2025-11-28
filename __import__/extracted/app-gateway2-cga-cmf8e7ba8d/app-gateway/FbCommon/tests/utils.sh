#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# shellcheck source=env.sh
source "${SCRIPT_DIR}/env.sh"

jsonrpc_post() {
  local method="$1"
  local params="${2:-{}}"
  local id="${3:-1}"
  local body
  body=$(printf '{"jsonrpc":"2.0","id":%s,"method":"%s","params":%s}' "${id}" "${method}" "${params}")
  ${CURL_BIN} -sS -H 'Content-Type: application/json' -d "${body}" "${APPGATEWAY_JSONRPC_URL}"
}

# Helper for event subscribe/unsubscribe
event_toggle() {
  local event_method="$1"
  local listen_bool="$2" # true|false
  jsonrpc_post "${event_method}" "{\"listen\": ${listen_bool}}"
}
