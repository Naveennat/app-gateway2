#!/usr/bin/env bash
# Start script for app-gateway2
# This provides a minimal, non-invasive start command so the preview system can launch the container.
# It serves the docs/ directory if present; otherwise serves the repository root.
# Respects the PORT environment variable (default 3000).

set -euo pipefail

PORT="${PORT:-3000}"

# Resolve repository root (one level up from this script)
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")"/.. && pwd)"
SERVE_DIR="$ROOT_DIR/docs"
if [ ! -d "$SERVE_DIR" ]; then
  SERVE_DIR="$ROOT_DIR"
fi

# Pick Python interpreter
PYTHON_BIN=""
if command -v python3 >/dev/null 2>&1; then
  PYTHON_BIN="python3"
elif command -v python >/dev/null 2>&1; then
  PYTHON_BIN="python"
else
  echo "Error: Python is required to run the development server (python3 or python) but was not found." >&2
  exit 1
fi

cd "$SERVE_DIR"
echo "app-gateway2: serving directory: $SERVE_DIR"
echo "app-gateway2: starting static server on port: $PORT"

# Use --bind only for python3; python2's SimpleHTTPServer does not support it.
if [ "$PYTHON_BIN" = "python3" ]; then
  exec "$PYTHON_BIN" -m http.server "$PORT" --bind 0.0.0.0
else
  # Fallback for legacy 'python' if it points to Python 2.x
  exec "$PYTHON_BIN" -m SimpleHTTPServer "$PORT"
fi
