#!/bin/bash

# Script: run_l0_with_custom_libpath_and_report.sh
# Purpose: Run L0 tests for AppGateway with a specific LD_LIBRARY_PATH pointing to installed Thunder/WPEFramework libs.
# Summarizes the test results and highlights loader/ABI errors.

set -e

BASE_DIR="$(dirname "$(readlink -f "$0")")"
DEPS_LIB="${BASE_DIR}/../../../dependencies/install/lib"
PLUGINS_LIB="${DEPS_LIB}/plugins"
# In this repo's vendored SDK, the directory "lib/wpeframework/plugins" may not exist.
# However "lib/wpeframework" (proxystubs) does exist and is relevant for runtime.
WPEFRAMEWORK_LIB="${DEPS_LIB}/wpeframework"
WPE_PLUGINS_LIB="${DEPS_LIB}/wpeframework/plugins"

# Compose LD_LIBRARY_PATH with only directories that exist (avoid confusing loader warnings).
LD_PATH=("${DEPS_LIB}")
[[ -d "${PLUGINS_LIB}" ]] && LD_PATH+=("${PLUGINS_LIB}")
[[ -d "${WPEFRAMEWORK_LIB}" ]] && LD_PATH+=("${WPEFRAMEWORK_LIB}")
[[ -d "${WPE_PLUGINS_LIB}" ]] && LD_PATH+=("${WPE_PLUGINS_LIB}")

export LD_LIBRARY_PATH="$(IFS=:; echo "${LD_PATH[*]}"):${LD_LIBRARY_PATH:-}"

L0_SCRIPT="${BASE_DIR}/run_coverage.sh"
LOG_FILE="${BASE_DIR}/l0_run_full.log"
SUMMARY_FILE="${BASE_DIR}/l0_run_summary.txt"

# Ensure artifacts and build directories are tied to the same per-run timestamp.
export RUN_ID="${RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)}"

echo "Running L0 tests with LD_LIBRARY_PATH: $LD_LIBRARY_PATH" > "$SUMMARY_FILE"
echo "Invoking L0 test runner..." | tee -a "$SUMMARY_FILE"

# Capture/propagate exit status so CI and callers correctly detect failures.
set +e
bash "$L0_SCRIPT" > "$LOG_FILE" 2>&1
RC=$?
set -e

echo "Test run complete. Exit code: $RC" | tee -a "$SUMMARY_FILE"
echo | tee -a "$SUMMARY_FILE"

echo "== L0 Test Results Summary ==" | tee -a "$SUMMARY_FILE"
grep -E "PASS|FAIL|Error|error|Loader|undefined|unsatisfied|symbol|crash|segmentation|Segmentation" "$LOG_FILE" | tee -a "$SUMMARY_FILE" || true

echo | tee -a "$SUMMARY_FILE"
echo "Full log is available at $LOG_FILE" | tee -a "$SUMMARY_FILE"
echo "Summary written to $SUMMARY_FILE"

exit "$RC"
