#!/usr/bin/env bash
#
# AppGateway standalone L0 tests + coverage
# ----------------------------------------
# Builds the standalone l0test target under:
#   $ROOT/app-gateway2/app-gateway/AppGateway/l0test/build
# runs the binary with Real-plugin runtime environment variables,
# then generates an lcov + genhtml report under:
#   $ROOT/coverage/index.html
#
# Assumptions (per request):
# - Repository root is the current working directory when executing:
#     ROOT=$(pwd)
#
# Notes:
# - This script is intended to be executable (chmod +x). We preserve the executable
#   bit by creating this file from an existing executable script template.

set -euo pipefail

log()  { echo "[run_l0_coverage] $*"; }
warn() { echo "[run_l0_coverage][WARN] $*" >&2; }
die()  { echo "[run_l0_coverage][ERROR] $*" >&2; exit 1; }

need_cmd() {
  local cmd="$1"
  if ! command -v "${cmd}" >/dev/null 2>&1; then
    return 1
  fi
  return 0
}

# Per requirement: ROOT is the current working directory.
ROOT="$(pwd)"

# Defensive fallback if user didn't run from repo root, but don't break the stated assumption.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXPECTED_L0_DIR="${ROOT}/app-gateway2/app-gateway/AppGateway/l0test"
if [[ ! -d "${EXPECTED_L0_DIR}" ]]; then
  ROOT_FALLBACK="$(cd "${SCRIPT_DIR}/../../../.." && pwd)"
  warn "Expected '${EXPECTED_L0_DIR}' does not exist. You may not be running from repo root."
  warn "Falling back to ROOT derived from script location: ${ROOT_FALLBACK}"
  ROOT="${ROOT_FALLBACK}"
fi

L0_DIR="${ROOT}/app-gateway2/app-gateway/AppGateway/l0test"
BUILD_DIR="${L0_DIR}/build"
INSTALL_PREFIX="${ROOT}/dependencies/install"

RESOLUTIONS_PATH="${ROOT}/app-gateway2/app-gateway/AppGateway/resolutions/resolution.base.json"

PLUGIN_CANDIDATE_1="${ROOT}/build/app-gateway/AppGateway/libWPEFrameworkAppGateway.so"
PLUGIN_CANDIDATE_2="${INSTALL_PREFIX}/lib/plugins/libWPEFrameworkAppGateway.so"

log "ROOT=${ROOT}"
log "L0_DIR=${L0_DIR}"
log "BUILD_DIR=${BUILD_DIR}"
log "CMAKE_PREFIX_PATH=${INSTALL_PREFIX}"

# ---- Tool checks ----
need_cmd cmake || die "cmake not found in PATH."
need_cmd ninja || die "ninja not found in PATH."
need_cmd genhtml || die "genhtml not found in PATH."

if ! need_cmd lcov; then
  echo "[run_l0_coverage][ERROR] lcov not found in PATH."
  echo "[run_l0_coverage][ERROR] Install hint: sudo apt install -y lcov"
  exit 1
fi

# ---- Input checks ----
[[ -d "${L0_DIR}" ]] || die "L0 test directory not found: ${L0_DIR}"
[[ -f "${L0_DIR}/CMakeLists.txt" ]] || die "CMakeLists.txt not found in: ${L0_DIR}"
[[ -d "${INSTALL_PREFIX}" ]] || die "Install prefix not found: ${INSTALL_PREFIX} (expected dependencies to be built/installed)"
[[ -f "${RESOLUTIONS_PATH}" ]] || die "Resolution base file not found: ${RESOLUTIONS_PATH}"

# ---- Plugin presence check (warn only) ----
if [[ -f "${PLUGIN_CANDIDATE_1}" ]]; then
  log "Found AppGateway plugin (build tree): ${PLUGIN_CANDIDATE_1}"
elif [[ -f "${PLUGIN_CANDIDATE_2}" ]]; then
  log "Found AppGateway plugin (installed): ${PLUGIN_CANDIDATE_2}"
else
  warn "AppGateway plugin not found at:"
  warn "  - ${PLUGIN_CANDIDATE_1}"
  warn "  - ${PLUGIN_CANDIDATE_2}"
  warn "Continuing anyway (tests may run in a mock-like configuration depending on environment)."
fi

# ---- Force coverage flags (requested) ----
# Force via environment variables AND pass into CMake cache to avoid stale-cache issues.
COVERAGE_CXXFLAGS="--coverage -O0 -g"
COVERAGE_LDFLAGS="--coverage"

export CXXFLAGS="${COVERAGE_CXXFLAGS} ${CXXFLAGS:-}"
export LDFLAGS="${COVERAGE_LDFLAGS} ${LDFLAGS:-}"

log "Using coverage flags:"
log "  CXXFLAGS=${CXXFLAGS}"
log "  LDFLAGS=${LDFLAGS}"

mkdir -p "${BUILD_DIR}"

# ---- Configure & build ----
log "Configuring (Debug) with Ninja generator..."
cmake -S "${L0_DIR}" -B "${BUILD_DIR}" -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" \
  -DPREFIX="${INSTALL_PREFIX}" \
  -DCMAKE_CXX_FLAGS="${COVERAGE_CXXFLAGS}" \
  -DCMAKE_EXE_LINKER_FLAGS="${COVERAGE_LDFLAGS}" \
  -DCMAKE_SHARED_LINKER_FLAGS="${COVERAGE_LDFLAGS}"

JOBS="4"
if command -v nproc >/dev/null 2>&1; then
  JOBS="$(nproc)"
fi

log "Building target 'appgateway_l0test' (jobs=${JOBS})..."
cmake --build "${BUILD_DIR}" --target appgateway_l0test -j "${JOBS}"

# ---- Locate test binary ----
CANDIDATES=(
  "${BUILD_DIR}/appgateway_l0test"
  "${BUILD_DIR}/AppGateway/appgateway_l0test"
)

TEST_BIN=""
for c in "${CANDIDATES[@]}"; do
  if [[ -x "${c}" ]]; then
    TEST_BIN="${c}"
    break
  fi
done

# Last resort: search in build dir
if [[ -z "${TEST_BIN}" ]]; then
  FOUND="$(find "${BUILD_DIR}" -maxdepth 4 -type f -name appgateway_l0test -perm -u+x 2>/dev/null | head -n 1 || true)"
  if [[ -n "${FOUND}" ]]; then
    TEST_BIN="${FOUND}"
  fi
fi

if [[ -z "${TEST_BIN}" ]]; then
  echo "[run_l0_coverage][ERROR] Test binary 'appgateway_l0test' not found after build." >&2
  echo "[run_l0_coverage][ERROR] Looked in:" >&2
  for c in "${CANDIDATES[@]}"; do
    echo "  - ${c}" >&2
  done
  echo "[run_l0_coverage][ERROR] Also searched under: ${BUILD_DIR} (maxdepth 4)" >&2
  exit 1
fi

log "Test binary: ${TEST_BIN}"

# ---- Run tests in 'Real plugin mode' environment (requested env vars) ----
# Include:
# - installed libs: $ROOT/dependencies/install/lib
# - l0test build dir (and possible AppGateway subdir) for runtime-loaded artifacts
LD_PATH_EXTRA=(
  "${INSTALL_PREFIX}/lib"
  "${BUILD_DIR}/AppGateway"
  "${BUILD_DIR}"
)

# Assemble LD_LIBRARY_PATH without introducing trailing ':' if empty
LD_LIBRARY_PATH_COMPOSED="$(IFS=:; echo "${LD_PATH_EXTRA[*]}")"
if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
  LD_LIBRARY_PATH_COMPOSED="${LD_LIBRARY_PATH_COMPOSED}:${LD_LIBRARY_PATH}"
fi

log "Running tests with:"
log "  LD_LIBRARY_PATH=${LD_LIBRARY_PATH_COMPOSED}"
log "  APPGATEWAY_RESOLUTIONS_PATH=${RESOLUTIONS_PATH}"

APPGATEWAY_RESOLUTIONS_PATH="${RESOLUTIONS_PATH}" \
LD_LIBRARY_PATH="${LD_LIBRARY_PATH_COMPOSED}" \
"${TEST_BIN}"

# ---- Coverage generation (requested commands) ----
log "Capturing coverage (lcov)..."
lcov -c -o "${BUILD_DIR}/coverage.info" -d "${BUILD_DIR}" --ignore-errors empty

log "Generating HTML report (genhtml)..."
mkdir -p "${ROOT}/coverage"
genhtml -o "${ROOT}/coverage" "${BUILD_DIR}/coverage.info"

log "Coverage report generated:"
echo "${ROOT}/coverage/index.html"
