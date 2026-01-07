#!/usr/bin/env bash
# Capture L0 artifacts into the per-run directory created by run_l0.sh.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"  # -> app-gateway2

# Args from run_l0.sh:
#   $1: build directory
#   $2: run directory
BUILD_DIR="${1:-}"
RUN_DIR="${2:-}"

if [[ -z "${BUILD_DIR}" || -z "${RUN_DIR}" ]]; then
  echo "Usage: $0 <build_dir> <run_dir>" >&2
  exit 2
fi

COVERAGE_DIR="${ROOT}/tests/l0/appgateway/coverage"

mkdir -p "${RUN_DIR}"

# Copy coverage artifacts if present
if [[ -d "${COVERAGE_DIR}" ]]; then
  cp -a "${COVERAGE_DIR}" "${RUN_DIR}/coverage"
fi

# Capture build dir metadata (useful for debugging missing gcda/gcno)
if [[ -d "${BUILD_DIR}" ]]; then
  mkdir -p "${RUN_DIR}/build_dir_snapshot"
  # Keep only lightweight text outputs
  (
    cd "${BUILD_DIR}"
    find . -maxdepth 3 -type f \( -name "*.log" -o -name "CMakeCache.txt" -o -name "build.ninja" \) -print0 \
      | xargs -0 -I{} bash -lc 'mkdir -p "'"${RUN_DIR}"'/build_dir_snapshot/$(dirname "{}")"; cp -a "{}" "'"${RUN_DIR}"'/build_dir_snapshot/{}"' || true
  )
fi

# Record basic environment info for reproducibility
{
  echo "timestamp_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "root=${ROOT}"
  echo "coverage_dir=${COVERAGE_DIR}"
  echo "build_dir=${BUILD_DIR}"
  echo "run_dir=${RUN_DIR}"
  echo "pwd=$(pwd)"
} > "${RUN_DIR}/meta.txt"

# Create tarball for convenience
tar -C "${RUN_DIR}" -czf "${RUN_DIR}/artifacts.tgz" .

echo "${RUN_DIR}"
echo "${RUN_DIR}/artifacts.tgz"
