#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "[tests] Running manifest test suite in ${ROOT_DIR}"

# 1) Build C++ test plugin if dependencies are present.
if [ -d "dependencies/install" ] && [ -f "app-gateway2_testing/CMakeLists.txt" ]; then
  echo "[tests] Building C++ test plugin (App2AppProviderTest)..."
  cmake -S app-gateway2_testing -B build-tests -G Ninja
  cmake --build build-tests
else
  echo "[tests] Skipping C++ plugin build (dependencies/install or app-gateway2_testing missing)."
fi

# 2) Node healthcheck smoke test (server starts and /health responds).
if command -v node >/dev/null 2>&1; then
  echo "[tests] Running Node healthcheck smoke test..."
  PORT="${PORT:-3000}"
  node index.js >/tmp/app-gateway2-test.log 2>&1 &
  PID=$!
  trap 'kill "$PID" >/dev/null 2>&1 || true' EXIT

  # Wait briefly for server.
  for _ in $(seq 1 20); do
    if curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null 2>&1; then
      echo "[tests] Healthcheck OK"
      break
    fi
    sleep 0.2
  done

  curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null
  kill "$PID" >/dev/null 2>&1 || true
  trap - EXIT
else
  echo "[tests] Warning: node not found; skipping Node smoke test." >&2
fi

echo "[tests] Done."
