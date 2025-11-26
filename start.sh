#!/usr/bin/env sh
# app-gateway2 start helper: run the preview server reliably
# - Installs dependencies if needed
# - Prefers npm start, falls back to node index.js
# - Binds to HOST/PORT environment variables with safe defaults
set -eu

# Always run from this script's directory
cd "$(dirname "$0")"

export HOST="${HOST:-0.0.0.0}"
export PORT="${PORT:-3000}"

# Install dependencies if npm is available and package.json exists
if command -v npm >/dev/null 2>&1 && [ -f package.json ]; then
  NEED_INSTALL=0
  if [ ! -d node_modules ]; then
    NEED_INSTALL=1
  elif [ ! -d node_modules/express ]; then
    NEED_INSTALL=1
  fi

  if [ "$NEED_INSTALL" -eq 1 ]; then
    echo "[app-gateway2] Installing dependencies (production only)..."
    if [ -f package-lock.json ]; then
      npm ci --omit=dev --no-audit --no-fund
    else
      npm install --omit=dev --no-audit --no-fund
    fi
  fi
fi

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
