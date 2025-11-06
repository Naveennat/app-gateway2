#!/usr/bin/env bash
set -euo pipefail

# Start script for app-gateway2 (C++/CMake-based Thunder plugins)
# - Configures and builds the project
# - Keeps a stable foreground process for preview systems by tailing logs
# - Optionally runs a local test binary once (non-blocking) if available

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build-local"
LOG_DIR="${ROOT_DIR}/.init"
THUNDER_LIB_DIR="${ROOT_DIR}/dependencies/install/lib"

mkdir -p "${BUILD_DIR}" "${LOG_DIR}"
: > "${LOG_DIR}/build.log"
: > "${LOG_DIR}/tests.log"

# Ensure runtime can locate Thunder shared libraries
export LD_LIBRARY_PATH="${THUNDER_LIB_DIR}:${LD_LIBRARY_PATH-}"

echo "[start.sh] Using build dir: ${BUILD_DIR}"
echo "[start.sh] Using Thunder lib dir: ${THUNDER_LIB_DIR}"
echo "[start.sh] LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"

# Configure with CMake if needed
if [ ! -f "${BUILD_DIR}/CMakeCache.txt" ]; then
  echo "[start.sh] Configuring CMake..."
  cmake -S "${ROOT_DIR}/app-gateway" -B "${BUILD_DIR}"
fi

# Build (do not exit the script if the build fails; keep the container running and stream logs)
echo "[start.sh] Building targets..."
set +e
cmake --build "${BUILD_DIR}" -- -j"$(nproc || echo 2)" 2>&1 | tee "${LOG_DIR}/build.log"
BUILD_EXIT=$?
set -e
echo "[start.sh] Build finished with status: ${BUILD_EXIT}"

# Optionally run a known test binary once (non-blocking)
LOCAL_TEST_BIN="${BUILD_DIR}/tests/test_appgateway"
if [ -x "${LOCAL_TEST_BIN}" ]; then
  echo "[start.sh] Running test binary once (non-blocking): ${LOCAL_TEST_BIN}"
  ("${LOCAL_TEST_BIN}" >> "${LOG_DIR}/tests.log" 2>&1 || true) &
fi

# Keep the process alive for preview systems by following logs
echo "[start.sh] Streaming logs. Press Ctrl+C to exit."
tail -n +1 -f "${LOG_DIR}/build.log" "${LOG_DIR}/tests.log"
