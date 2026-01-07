#!/usr/bin/env bash
set -euo pipefail

# Minimal artifact capture for L0 runs.
#
# Usage:
#   capture_artifacts.sh <build_dir> <run_dir>
#
# Captures:
#  - CMakeCache.txt
#  - CTest logs (if present)
#  - CTestTestfile.cmake files
#  - test binaries (if present)
#  - built shared libraries (if present)

if [[ $# -ne 2 ]]; then
  echo "Usage: $0 <build_dir> <run_dir>" >&2
  exit 2
fi

BUILD_DIR="$1"
RUN_DIR="$2"

mkdir -p "${RUN_DIR}"

# Capture basic build metadata
if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
  cp -f "${BUILD_DIR}/CMakeCache.txt" "${RUN_DIR}/" || true
fi

# Capture CTest logs
if [[ -d "${BUILD_DIR}/Testing" ]]; then
  mkdir -p "${RUN_DIR}/Testing"
  cp -a "${BUILD_DIR}/Testing" "${RUN_DIR}/" || true
fi

# Capture generated CTestTestfile.cmake (helps diagnose discovery issues)
mkdir -p "${RUN_DIR}/ctest_files"
find "${BUILD_DIR}" -name "CTestTestfile.cmake" -maxdepth 6 -print -exec cp -f {} "${RUN_DIR}/ctest_files/" \; 2>/dev/null || true

# Capture test binaries (common locations)
mkdir -p "${RUN_DIR}/binaries"
if [[ -d "${BUILD_DIR}/tests" ]]; then
  find "${BUILD_DIR}/tests" -maxdepth 1 -type f -executable -print -exec cp -f {} "${RUN_DIR}/binaries/" \; 2>/dev/null || true
fi

# Capture shared libs if present
mkdir -p "${RUN_DIR}/shared_libs"
find "${BUILD_DIR}" -maxdepth 4 -type f \( -name "*.so" -o -name "*.so.*" \) -print -exec cp -f {} "${RUN_DIR}/shared_libs/" \; 2>/dev/null || true

# Always write a short manifest
{
  echo "build_dir=${BUILD_DIR}"
  echo "captured_at_utc=$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  echo "uname=$(uname -a)"
} > "${RUN_DIR}/manifest.txt"
