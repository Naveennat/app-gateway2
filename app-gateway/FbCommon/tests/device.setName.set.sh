#!/usr/bin/env bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/utils.sh"

VALUE="${1:-Living Room}"
jsonrpc_post "device.setName" "{\"value\":\"${VALUE}\"}"
echo
