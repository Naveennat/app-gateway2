#!/usr/bin/env sh
# app-gateway2 scripts/start.sh
# Purpose:
# - Build (if needed) and start the service for preview.
# - Prefer Node (npm run start), fallback to node server.js/index.js, else run a built C++ binary in ./build.
# Notes:
# - Executable bit hint: this script is intended to be executable (chmod +x), the platform will set perms.

set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

export HOST="${HOST:-0.0.0.0}"
export PORT="${PORT:-3000}"

echo "[start] Running build steps ..."
if [ -x scripts/build.sh ]; then
  sh scripts/build.sh
else
  echo "[start] Note: scripts/build.sh not found or not executable; continuing." >&2
fi

# Try Node.js startup first
if [ -f package.json ]; then
  if command -v npm >/dev/null 2>&1; then
    echo "[start] Starting Node app via: npm run start (HOST=${HOST} PORT=${PORT})"
    exec npm run start
  elif command -v node >/dev/null 2>&1; then
    if [ -f server.js ]; then
      echo "[start] npm not available; starting via: node server.js (HOST=${HOST} PORT=${PORT})"
      exec node server.js
    elif [ -f index.js ]; then
      echo "[start] npm not available; starting via: node index.js (HOST=${HOST} PORT=${PORT})"
      exec node index.js
    fi
  fi
fi

# If no Node target, try a built C++ binary in ./build
if [ -d build ]; then
  # Try to find an executable file in build root
  BIN="$(find build -maxdepth 1 -type f -perm -u+x -print 2>/dev/null | head -n1 || true)"
  if [ -n "${BIN:-}" ]; then
    echo "[start] Starting built binary: ${BIN}"
    exec "${BIN}"
  fi
fi

echo "[start] Error: No runnable target found. Expected a Node app (package.json) or a built binary in ./build." >&2
exit 1
