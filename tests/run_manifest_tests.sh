#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

ARTIFACTS_DIR="${ARTIFACTS_DIR:-${ROOT_DIR}/artifacts}"
RESULTS_DIR="${ARTIFACTS_DIR}/test-results"
COVERAGE_DIR="${ARTIFACTS_DIR}/coverage"
mkdir -p "$RESULTS_DIR" "$COVERAGE_DIR"

echo "[tests] Running manifest test suite in ${ROOT_DIR}"
echo "[tests] Artifacts: ${ARTIFACTS_DIR}"

TOTAL=0
PASSED=0
FAILED=0
SKIPPED=0

run_step() {
  local name="$1"
  shift
  local log="${RESULTS_DIR}/${name}.log"
  TOTAL=$((TOTAL + 1))
  echo "[tests] === ${name} ==="
  set +e
  "$@" >"$log" 2>&1
  local rc=$?
  set -e
  if [ $rc -eq 0 ]; then
    PASSED=$((PASSED + 1))
    echo "[tests] PASS: ${name}"
  else
    FAILED=$((FAILED + 1))
    echo "[tests] FAIL: ${name} (rc=${rc})"
    echo "[tests] --- tail ${log} ---"
    tail -n 50 "$log" || true
    echo "[tests] --- end tail ---"
  fi
  return 0
}

skip_step() {
  local name="$1"
  local reason="$2"
  local log="${RESULTS_DIR}/${name}.log"
  TOTAL=$((TOTAL + 1))
  SKIPPED=$((SKIPPED + 1))
  {
    echo "[tests] SKIP: ${name}"
    echo "[tests] Reason: ${reason}"
  } >"$log"
  echo "[tests] SKIP: ${name} - ${reason}"
}

# 1) Build C++ test plugin only if Thunder dev headers exist.
# The repo may contain only Thunder generator tooling (Modules/*.cmake) without core/plugin headers.
if [ -f "app-gateway2_testing/CMakeLists.txt" ]; then
  if [ -f "dependencies/install/include/WPEFramework/core/Core.h" ] || [ -f "dependencies/install/include/WPEFramework/core/core.h" ]; then
    run_step "cpp_app2appprovider_build" cmake -S app-gateway2_testing -B build-tests -G Ninja
    run_step "cpp_app2appprovider_compile" cmake --build build-tests
  else
    skip_step "cpp_app2appprovider_build" "Thunder dev headers missing (expected core/Core.h under dependencies/install/include/WPEFramework)."
  fi
else
  skip_step "cpp_app2appprovider_build" "app-gateway2_testing/CMakeLists.txt missing."
fi

# 2) CTest for prebuilt build-ottservices tests, only if runtime libs exist.
if [ -f "build-ottservices/tests/CTestTestfile.cmake" ]; then
  # If lib dependencies are missing, the binary will fail to load. Don't hard-fail the suite for missing dev/runtime libs.
  if ldd build-ottservices/tests/test_appgateway 2>/dev/null | grep -q "not found"; then
    skip_step "cpp_ctest_build_ottservices" "build-ottservices/tests binaries have missing shared libraries (ldd shows 'not found')."
  else
    run_step "cpp_ctest_build_ottservices" bash -lc "cd build-ottservices/tests && ctest --output-on-failure"
  fi
else
  skip_step "cpp_ctest_build_ottservices" "No CTest suite found under build-ottservices/tests."
fi

# 3) Node healthcheck smoke test (server starts and /health responds).
if command -v node >/dev/null 2>&1; then
  # Ensure production deps are present (express) so node index.js can start in CI.
  if command -v npm >/dev/null 2>&1 && [ -f package.json ]; then
    if [ ! -d node_modules ] || [ ! -d node_modules/express ]; then
      run_step "node_install_deps" bash -lc '
        set -euo pipefail
        npm ci --omit=dev --no-audit --no-fund
      '
    fi
  fi

  # Inline runner so we can reliably capture logs/artifacts.
  run_step "node_healthcheck" bash -lc '
    set -euo pipefail
    PORT="${PORT:-3000}"
    node index.js >"'"${RESULTS_DIR}"'/node_server.log" 2>&1 &
    PID=$!
    trap "kill \"${PID}\" >/dev/null 2>&1 || true" EXIT

    for _ in $(seq 1 40); do
      if curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null 2>&1; then
        echo "Healthcheck OK"
        break
      fi
      sleep 0.25
    done

    curl -fsS "http://127.0.0.1:${PORT}/health" >/dev/null
    kill "$PID" >/dev/null 2>&1 || true
    trap - EXIT
  '
else
  skip_step "node_healthcheck" "node not found."
fi

# 4) Coverage collection (lcov) if any gcda/gcno exist.
# Best-effort: do not fail the suite if the workspace has no execution data (.gcda).
if command -v lcov >/dev/null 2>&1; then
  if find . -type f \( -name "*.gcda" -o -name "*.gcno" \) | grep -q .; then
    run_step "coverage_lcov" bash -lc '
      set -euo pipefail
      lcov --capture --directory . --output-file "'"${COVERAGE_DIR}"'/lcov.info" --ignore-errors mismatch,unused,empty || true

      # If lcov produced an empty tracefile, treat as "no data" (non-fatal) but keep artifacts.
      if ! grep -q "SF:" "'"${COVERAGE_DIR}"'/lcov.info" 2>/dev/null; then
        echo "No valid coverage records found (no SF: entries). Keeping lcov.info for artifacts."
        exit 0
      fi

      genhtml "'"${COVERAGE_DIR}"'/lcov.info" --output-directory "'"${COVERAGE_DIR}"'/html" --ignore-errors empty >/dev/null
      echo "lcov report written to '"${COVERAGE_DIR}"'/lcov.info and '"${COVERAGE_DIR}"'/html/"
    '
  else
    skip_step "coverage_lcov" "No gcda/gcno files present; nothing to capture."
  fi
else
  skip_step "coverage_lcov" "lcov not installed."
fi

# Final summary artifact
SUMMARY_FILE="${RESULTS_DIR}/summary.txt"
{
  echo "total=${TOTAL}"
  echo "passed=${PASSED}"
  echo "failed=${FAILED}"
  echo "skipped=${SKIPPED}"
} >"$SUMMARY_FILE"

echo "[tests] Summary: total=${TOTAL} passed=${PASSED} failed=${FAILED} skipped=${SKIPPED}"
echo "[tests] Done."

# Overall exit: fail if any steps failed
if [ "${FAILED}" -ne 0 ]; then
  exit 1
fi
