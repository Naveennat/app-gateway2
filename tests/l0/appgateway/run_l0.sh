#!/usr/bin/env bash
set -euo pipefail

# L0 runner for app-gateway2 (AppGateway C++ component)
#
# This script:
#  1) Creates a timestamped artifacts run directory
#  2) Configures a CMake build directory (default: build-ottservices) against app-gateway/
#  3) Builds targets
#  4) Runs CTest with verbose output
#  5) Captures artifacts into the run directory
#
# Usage:
#   ./tests/l0/appgateway/run_l0.sh [--build-dir <dir>] [--clean]
#

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
ARTIFACTS_DIR="${SCRIPT_DIR}/artifacts"

BUILD_DIR="${REPO_ROOT}/build-ottservices"
CLEAN=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      BUILD_DIR="$2"
      shift 2
      ;;
    --clean)
      CLEAN=1
      shift
      ;;
    *)
      echo "Unknown argument: $1" >&2
      exit 2
      ;;
  esac
done

TS="$(date -u +%Y%m%dT%H%M%SZ)"
RUN_DIR="${ARTIFACTS_DIR}/run_${TS}"
mkdir -p "${RUN_DIR}"

if [[ "${CLEAN}" -eq 1 ]]; then
  rm -rf "${BUILD_DIR}"
fi
mkdir -p "${BUILD_DIR}"

# Configure from the real project root (app-gateway/)
cmake -S "${REPO_ROOT}/app-gateway" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DNAMESPACE=WPEFramework

cmake --build "${BUILD_DIR}" -j"$(nproc)"

# Ensure runtime can resolve Thunder/WPEFramework shared libraries.
export LD_LIBRARY_PATH="${REPO_ROOT}/dependencies/install/lib:${LD_LIBRARY_PATH:-}"

# Run tests; keep going to capture logs even on failure
set +e
ctest --test-dir "${BUILD_DIR}" --output-on-failure -V | tee "${RUN_DIR}/ctest_output.txt"
CTEST_RC="${PIPESTATUS[0]}"
set -e

# Capture artifacts
"${SCRIPT_DIR}/capture_artifacts.sh" "${BUILD_DIR}" "${RUN_DIR}" || true

# Update LATEST_RUN pointer
echo "tests/l0/appgateway/artifacts/run_${TS}/" > "${ARTIFACTS_DIR}/LATEST_RUN.txt"

# Propagate ctest exit code
exit "${CTEST_RC}"
