#!/bin/bash

# Script: run_l0_with_custom_libpath_and_report.sh
# Purpose: Run L0 tests for AppGateway with a specific LD_LIBRARY_PATH pointing to installed Thunder/WPEFramework libs.
# Summarizes the test results and highlights loader/ABI errors.

set -e

BASE_DIR="$(dirname "$(readlink -f "$0")")"
DEPS_LIB="${BASE_DIR}/../../../dependencies/install/lib"
PLUGINS_LIB="${DEPS_LIB}/plugins"
WPE_PLUGINS_LIB="${DEPS_LIB}/wpeframework/plugins"

export LD_LIBRARY_PATH="${DEPS_LIB}:${PLUGINS_LIB}:${WPE_PLUGINS_LIB}:${LD_LIBRARY_PATH:-}"

L0_SCRIPT="${BASE_DIR}/run_coverage.sh"
LOG_FILE="${BASE_DIR}/l0_run_full.log"
SUMMARY_FILE="${BASE_DIR}/l0_run_summary.txt"

echo "Running L0 tests with LD_LIBRARY_PATH: $LD_LIBRARY_PATH" > "$SUMMARY_FILE"
echo "Invoking L0 test runner..." | tee -a "$SUMMARY_FILE"
bash "$L0_SCRIPT" > "$LOG_FILE" 2>&1

echo "Test run complete." | tee -a "$SUMMARY_FILE"
echo | tee -a "$SUMMARY_FILE"

echo "== L0 Test Results Summary ==" | tee -a "$SUMMARY_FILE"
grep -E "PASS|FAIL|Error|error|Loader|undefined|unsatisfied|symbol|crash|segmentation|Segmentation" "$LOG_FILE" | tee -a "$SUMMARY_FILE"

echo | tee -a "$SUMMARY_FILE"
echo "Full log is available at $LOG_FILE" | tee -a "$SUMMARY_FILE"
echo "Summary written to $SUMMARY_FILE"
