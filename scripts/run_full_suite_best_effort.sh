#!/usr/bin/env bash
set -euo pipefail

# Best-effort full-suite runner.
# - Delegates to tests/run_manifest_tests.sh (which already skips missing C++/Thunder SDK).
# - Persists a stable exit code artifact.
# - Produces a concise markdown summary (pass/fail/skip + coverage presence).
#
# PUBLIC_INTERFACE
# Usage:
#   ./scripts/run_full_suite_best_effort.sh
#
# Artifacts:
#   artifacts/test-results/** (logs + summary.txt + exit code)
#   artifacts/coverage/** (lcov.info + html/ if available)

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

ARTIFACTS_DIR="${ARTIFACTS_DIR:-${ROOT_DIR}/artifacts}"
RESULTS_DIR="${ARTIFACTS_DIR}/test-results"
COVERAGE_DIR="${ARTIFACTS_DIR}/coverage"
mkdir -p "$RESULTS_DIR" "$COVERAGE_DIR"

CONSOLE_LOG="${RESULTS_DIR}/run_full_suite_best_effort.console.log"
EXIT_CODE_FILE="${RESULTS_DIR}/manifest_tests.exit_code"
SUMMARY_TXT="${RESULTS_DIR}/summary.txt"
SUMMARY_MD="${RESULTS_DIR}/summary.md"

set +e
./tests/run_manifest_tests.sh >"$CONSOLE_LOG" 2>&1
RC=$?
set -e

echo "$RC" >"$EXIT_CODE_FILE"

# Determine whether any meaningful coverage was produced (lcov.info with at least one SF:)
COVERAGE_STATUS="unavailable"
if [ -f "${COVERAGE_DIR}/lcov.info" ]; then
  if grep -q "^SF:" "${COVERAGE_DIR}/lcov.info" 2>/dev/null; then
    COVERAGE_STATUS="present"
  else
    COVERAGE_STATUS="empty"
  fi
fi

# Build concise markdown summary from summary.txt (if present)
TOTAL="?"
PASSED="?"
FAILED="?"
SKIPPED="?"

if [ -f "$SUMMARY_TXT" ]; then
  # shellcheck disable=SC1090
  source "$SUMMARY_TXT" || true
  TOTAL="${total:-$TOTAL}"
  PASSED="${passed:-$PASSED}"
  FAILED="${failed:-$FAILED}"
  SKIPPED="${skipped:-$SKIPPED}"
fi

{
  echo "# Test Summary (best-effort)"
  echo
  echo "- exit_code: \`$RC\`"
  echo "- total: \`$TOTAL\`"
  echo "- passed: \`$PASSED\`"
  echo "- failed: \`$FAILED\`"
  echo "- skipped: \`$SKIPPED\`"
  echo "- coverage: \`$COVERAGE_STATUS\` (artifacts/coverage/lcov.info)"
  echo
  echo "## Notes"
  echo "- C++ tests are skipped when Thunder/WPEFramework SDK headers/libs are missing."
  echo "- Coverage is best-effort; an empty lcov.info indicates no gcda execution data was available."
} >"$SUMMARY_MD"

# Do not mask the underlying suite result.
exit "$RC"
