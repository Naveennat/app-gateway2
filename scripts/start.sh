#!/usr/bin/env bash
set -euo pipefail

# Start script for app-gateway2 (C++/CMake-based Thunder plugins)
# - Configures and builds the project
# - If a known runnable binary exists, runs it so the preview system has a foreground process
# - Otherwise, tails the build log to keep the process alive and provide diagnostics

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-local"
LOG_DIR="${ROOT_DIR}/.init"
mkdir -p "${BUILD_DIR}" "${LOG_DIR}"

echo "[start.sh] Using build dir: ${BUILD_DIR}"

# Configure with CMake
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  echo "[start.sh] Configuring CMake..."
  cmake -S "${ROOT_DIR}/app-gateway" -B "${BUILD_DIR}"
fi

# Build
echo "[start.sh] Building targets..."
cmake --build "${BUILD_DIR}" -- -j"$(nproc || echo 2)" | tee "${LOG_DIR}/build.log"

# Known runnable test from prior builds (if present)
TEST_BIN="${ROOT_DIR}/build-ottservices/tests/test_appgateway"
LOCAL_TEST_BIN="${BUILD_DIR}/tests/test_appgateway"

if [ -x "${LOCAL_TEST_BIN}" ]; then
  echo "[start.sh] Running local test binary: ${LOCAL_TEST_BIN}"
  exec "${LOCAL_TEST_BIN}"
elif [ -x "${TEST_BIN}" ]; then
  echo "[start.sh] Running prebuilt test binary: ${TEST_BIN}"
  exec "${TEST_BIN}"
else
  echo "[start.sh] No runnable binary found (e.g., tests/test_appgateway)."
  echo "[start.sh] The project produces Thunder plugin shared libraries intended to be loaded by a WPEFramework (Thunder) daemon."
  echo "[start.sh] Build logs will be tailed to keep the process alive for the preview system."
  echo "[start.sh] Tip: Integrate with a local Thunder instance and load plugins from app-gateway/*."
  tail -f "${LOG_DIR}/build.log"
fi
