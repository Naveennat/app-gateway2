#!/usr/bin/env bash
# Capture L0 coverage artifacts produced by run_coverage.sh into a stable folder.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"  # -> app-gateway2

COVERAGE_DIR="${ROOT}/tests/l0/appgateway/coverage"
BUILD_DIR="${ROOT}/build/tests_l0_appgateway"
ARTIFACTS_DIR="${ROOT}/tests/l0/appgateway/artifacts"

mkdir -p "${ARTIFACTS_DIR}"

# Copy coverage artifacts if present
if [[ -d "${COVERAGE_DIR}" ]]; then
  rm -rf "${ARTIFACTS_DIR}/coverage"
  cp -a "${COVERAGE_DIR}" "${ARTIFACTS_DIR}/coverage"
fi

# Capture build dir metadata (useful for debugging missing gcda/gcno)
if [[ -d "${BUILD_DIR}" ]]; then
  rm -rf "${ARTIFACTS_DIR}/build_dir_snapshot"
  mkdir -p "${ARTIFACTS_DIR}/build_dir_snapshot"
  # Keep only lightweight text outputs
  (cd "${BUILD_DIR}" && find . -maxdepth 3 -type f \( -name "*.log" -o -name "CMakeCache.txt" -o -name "build.ninja" \) -print0 | xargs -0 -I{} bash -lc 'mkdir -p "'"${ARTIFACTS_DIR}"'/build_dir_snapshot/$(dirname "{}")"; cp -a "{}" "'"${ARTIFACTS_DIR}"'/build_dir_snapshot/{}"' || true
fi

# Record basic environment info for reproducibility
{
  echo "timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "root=${ROOT}"
  echo "coverage_dir=${COVERAGE_DIR}"
  echo "build_dir=${BUILD_DIR}"
  echo "pwd=$(pwd)"
} > "${ARTIFACTS_DIR}/meta.txt"

# Create tarball for convenience
tar -C "${ARTIFACTS_DIR}" -czf "${ARTIFACTS_DIR}/artifacts.tgz" .

echo "${ARTIFACTS_DIR}"
echo "${ARTIFACTS_DIR}/artifacts.tgz"
