#!/usr/bin/env bash
set -euo pipefail

# Ensure the fallback config path expected by the plugin exists in CI.
# The plugin tries:
#   /etc/app-gateway/resolutions.json (may not exist in isolated build)
#   /etc/app-gateway/resolution.base.json (fallback)
#
# For L0 tests we make sure the fallback exists so Resolver can load a baseline.
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_BASE="${ROOT_DIR}/plugin/AppGateway/resolutions/resolution.base.json"
DST_DIR="/etc/app-gateway"
DST_BASE="${DST_DIR}/resolution.base.json"

if [[ ! -f "${SRC_BASE}" ]]; then
  echo "[ensure_test_configs][ERROR] Missing repo base resolution file: ${SRC_BASE}" >&2
  exit 1
fi

sudo mkdir -p "${DST_DIR}"
sudo cp -f "${SRC_BASE}" "${DST_BASE}"

echo "[ensure_test_configs] Installed ${DST_BASE} from ${SRC_BASE}"
