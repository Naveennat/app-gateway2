#!/usr/bin/env sh
# app-gateway2 start helper: run the preview server reliably
# - Prefers npm start, falls back to node server.js
# - Binds to HOST/PORT environment variables with safe defaults
set -eu

# Always run from this script's directory
cd "$(dirname "$0")"

export HOST="${HOST:-0.0.0.0}"
export PORT="${PORT:-3000}"

if command -v npm >/dev/null 2>&1; then
  echo "[app-gateway2] Starting via npm start on ${HOST}:${PORT} ..."
  exec npm start
elif command -v node >/dev/null 2>&1; then
  echo "[app-gateway2] npm not found, starting via node index.js on ${HOST}:${PORT} ..."
  exec node index.js
else
  echo "[app-gateway2] Error: neither npm nor node is available in PATH." >&2
  exit 1
fi
