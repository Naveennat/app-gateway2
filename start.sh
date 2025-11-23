#!/usr/bin/env sh
# Start script for app-gateway2 - provides an alternative entrypoint for preview systems.
# Ensures HOST and PORT are set and invokes npm start.

set -eu

export HOST="${HOST:-0.0.0.0}"
export PORT="${PORT:-3000}"

echo "[app-gateway2] Using HOST=${HOST} PORT=${PORT}"
echo "[app-gateway2] Executing: npm start"
exec npm start
