#!/usr/bin/env bash
#
# AppGateway standalone L0 tests + coverage
# ----------------------------------------
# This script:
#  - Cleans and rebuilds the standalone l0test target (Ninja generator)
#  - Forces coverage compilation/link flags
#  - Runs the l0tests in "Real plugin mode" runtime environment
#  - Generates an lcov + genhtml report under:
#      app-gateway2/AppGateway/l0test/coverage/index.html
#
# Requirements satisfied by this script (see task description):
#  1) rm -rf the l0test build directory before configure
#  2) Reconfigure using Ninja generator pointing to standalone l0test CMakeLists.txt
#  3) Build with coverage flags: CXXFLAGS='--coverage -O0 -g' and LDFLAGS='--coverage'
#  4) Run l0tests with Real plugin mode defaults:
#       - preferred plugin: ./build/app-gateway/AppGateway/libWPEFrameworkAppGateway.so
#       - fallback plugin:  ./dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so
#       - APPGATEWAY_RESOLUTIONS_PATH defaults to:
#           app-gateway2/AppGateway/resolutions/resolution.base.json
#     (If env vars are already set, they are preserved.)
#  5) Coverage:
#       - info file: app-gateway2/AppGateway/l0test/build/coverage.info
#       - html dir:  app-gateway2/AppGateway/l0test/coverage
#  6) Clear logs + final HTML path printed
#  7) Robust error handling (set -euo pipefail). Script is intended to be executable.

set -euo pipefail

log()        { echo "[run_l0_coverage] $*"; }
log_section(){ echo ""; echo "[run_l0_coverage] ===== $* ====="; }
warn()       { echo "[run_l0_coverage][WARN] $*" >&2; }
die()        { echo "[run_l0_coverage][ERROR] $*" >&2; exit 1; }

need_cmd() {
  local cmd="$1"
  command -v "${cmd}" >/dev/null 2>&1
}

# NOTE: In some CI/agent environments, the working directory may not be the repo root
# even if the command is invoked from it. Derive ROOT from this script's location
# to make the script robust.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"

L0_DIR="${ROOT}/app-gateway2/AppGateway/l0test"
BUILD_DIR="${L0_DIR}/build"
COVERAGE_DIR="${L0_DIR}/coverage"
# Repo layout note:
# Dependencies are vendored under app-gateway2/dependencies (not at repo-root).
INSTALL_PREFIX="${ROOT}/app-gateway2/dependencies/install"

# ---- Thunder/WPEFramework SDK (R4_4 / 4.4) resolution ----
# Force CMake and pkg-config to pick SDK artifacts from our local install prefix.
# This is critical to avoid mixing headers/libs from other Thunder versions present on the system.
export CMAKE_PREFIX_PATH="${INSTALL_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

DEFAULT_RESOLUTIONS_PATH="${ROOT}/app-gateway2/AppGateway/resolutions/resolution.base.json"

DEFAULT_PLUGIN_PREFERRED="${ROOT}/build/app-gateway/AppGateway/libWPEFrameworkAppGateway.so"
DEFAULT_PLUGIN_FALLBACK="${ROOT}/dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so"

log "ROOT=${ROOT}"
log "L0_DIR=${L0_DIR}"
log "BUILD_DIR=${BUILD_DIR}"
log "INSTALL_PREFIX=${INSTALL_PREFIX}"
log "COVERAGE_DIR=${COVERAGE_DIR}"

# ---- Tool checks ----
log_section "Tool checks"
need_cmd cmake   || die "cmake not found in PATH."
need_cmd ninja   || die "ninja not found in PATH."
need_cmd lcov    || die "lcov not found in PATH. Install hint: sudo apt install -y lcov"
need_cmd genhtml || die "genhtml not found in PATH. Install hint: sudo apt install -y lcov"

# ---- Input checks ----
log_section "Input checks"
[[ -d "${L0_DIR}" ]] || die "L0 test directory not found: ${L0_DIR}"
[[ -f "${L0_DIR}/CMakeLists.txt" ]] || die "CMakeLists.txt not found in: ${L0_DIR}"
[[ -d "${INSTALL_PREFIX}" ]] || die "Install prefix not found: ${INSTALL_PREFIX} (expected dependencies to be built/installed)"

# Preserve env var if already set; otherwise default it to the required file.
RESOLUTIONS_PATH="${APPGATEWAY_RESOLUTIONS_PATH:-${DEFAULT_RESOLUTIONS_PATH}}"
[[ -f "${RESOLUTIONS_PATH}" ]] || die "Resolution base file not found: ${RESOLUTIONS_PATH}"

# Select plugin path:
# - If APPGATEWAY_PLUGIN_SO is set externally, preserve it.
# - Else pick preferred, then fallback (as requested).
APPGATEWAY_PLUGIN_SO="${APPGATEWAY_PLUGIN_SO:-}"
if [[ -z "${APPGATEWAY_PLUGIN_SO}" ]]; then
  if [[ -f "${DEFAULT_PLUGIN_PREFERRED}" ]]; then
    APPGATEWAY_PLUGIN_SO="${DEFAULT_PLUGIN_PREFERRED}"
  elif [[ -f "${DEFAULT_PLUGIN_FALLBACK}" ]]; then
    APPGATEWAY_PLUGIN_SO="${DEFAULT_PLUGIN_FALLBACK}"
  else
    # Default to preferred path even if missing, for consistency in logs.
    APPGATEWAY_PLUGIN_SO="${DEFAULT_PLUGIN_PREFERRED}"
  fi
fi

if [[ -f "${APPGATEWAY_PLUGIN_SO}" ]]; then
  log "Using AppGateway plugin .so: ${APPGATEWAY_PLUGIN_SO}"
else
  warn "AppGateway plugin .so not found at selected path: ${APPGATEWAY_PLUGIN_SO}"
  warn "Also expected either:"
  warn "  - ${DEFAULT_PLUGIN_PREFERRED}"
  warn "  - ${DEFAULT_PLUGIN_FALLBACK}"
  warn "Continuing anyway (tests may run in pure-mock mode)."
fi

# ---- Coverage flags (requested) ----
# Preserve existing flags but ensure coverage flags are present.
log_section "Coverage flags"
COVERAGE_CXXFLAGS="--coverage -O0 -g"
COVERAGE_LDFLAGS="--coverage"

export CXXFLAGS="${COVERAGE_CXXFLAGS}${CXXFLAGS:+ ${CXXFLAGS}}"
export LDFLAGS="${COVERAGE_LDFLAGS}${LDFLAGS:+ ${LDFLAGS}}"

log "CXXFLAGS=${CXXFLAGS}"
log "LDFLAGS=${LDFLAGS}"

# ---- Clean build dir (requested) ----
log_section "Clean build directory"
# Safety: only ever delete the canonical l0test build directory.
if [[ "${BUILD_DIR}" != "${L0_DIR}/build" ]]; then
  die "Refusing to clean unexpected build directory. BUILD_DIR='${BUILD_DIR}' L0_DIR='${L0_DIR}'"
fi

log "rm -rf '${BUILD_DIR}'"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

# ---- Help l0test CMake find the preferred plugin without changing any CMake files ----
# l0test/CMakeLists.txt probes:
#   ${CMAKE_BINARY_DIR}/../AppGateway/libWPEFrameworkAppGateway.so
# With BUILD_DIR=${L0_DIR}/build that becomes:
#   ${L0_DIR}/AppGateway/libWPEFrameworkAppGateway.so
# So if we have a real plugin .so (preferred or fallback), create a symlink there.
log_section "Prepare Real-plugin link path (optional)"
if [[ -f "${APPGATEWAY_PLUGIN_SO}" ]]; then
  mkdir -p "${L0_DIR}/AppGateway"
  ln -sf "${APPGATEWAY_PLUGIN_SO}" "${L0_DIR}/AppGateway/libWPEFrameworkAppGateway.so"
  log "Symlinked for CMake probe:"
  log "  ${L0_DIR}/AppGateway/libWPEFrameworkAppGateway.so -> ${APPGATEWAY_PLUGIN_SO}"
else
  warn "Skipping plugin symlink (plugin .so missing)."
fi

# ---- Configure & build (Ninja, standalone l0test CMake) ----
log_section "Configure (Ninja) and build"
log "Configuring with Ninja generator against: ${L0_DIR}"
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
log_section "Locate test binary"
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

[[ -n "${TEST_BIN}" ]] || die "Test binary 'appgateway_l0test' not found after build (searched maxdepth 4 under ${BUILD_DIR})."
log "Test binary: ${TEST_BIN}"

# ---- Run tests (Real plugin mode env) ----
log_section "Run tests"
# Ensure runtime can find:
# - Thunder libs: dependencies/install/lib
# - l0test build output: build/AppGateway (where the l0test CMake may emit artifacts)
# - plugin directory (preferred or fallback) to satisfy dynamic loader for the real plugin
PLUGIN_DIR="$(dirname "${APPGATEWAY_PLUGIN_SO}")"

LD_PATH_EXTRA=(
  "${INSTALL_PREFIX}/lib"
  "${INSTALL_PREFIX}/lib/plugins"
  "${BUILD_DIR}/AppGateway"
  "${BUILD_DIR}"
)

if [[ -n "${PLUGIN_DIR}" && -d "${PLUGIN_DIR}" ]]; then
  LD_PATH_EXTRA=( "${PLUGIN_DIR}" "${LD_PATH_EXTRA[@]}" )
fi

LD_LIBRARY_PATH_COMPOSED="$(IFS=:; echo "${LD_PATH_EXTRA[*]}")"
if [[ -n "${LD_LIBRARY_PATH:-}" ]]; then
  LD_LIBRARY_PATH_COMPOSED="${LD_LIBRARY_PATH_COMPOSED}:${LD_LIBRARY_PATH}"
fi

log "Environment:"
log "  APPGATEWAY_RESOLUTIONS_PATH=${RESOLUTIONS_PATH}"
log "  APPGATEWAY_PLUGIN_SO=${APPGATEWAY_PLUGIN_SO}"
log "  LD_LIBRARY_PATH=${LD_LIBRARY_PATH_COMPOSED}"

APPGATEWAY_RESOLUTIONS_PATH="${RESOLUTIONS_PATH}" \
APPGATEWAY_PLUGIN_SO="${APPGATEWAY_PLUGIN_SO}" \
LD_LIBRARY_PATH="${LD_LIBRARY_PATH_COMPOSED}" \
"${TEST_BIN}"

# ---- Coverage generation (lcov + genhtml) ----
log_section "Generate coverage"
log "Capturing coverage to: ${BUILD_DIR}/coverage.info"
lcov -c -o "${BUILD_DIR}/coverage.info" -d "${BUILD_DIR}" --ignore-errors empty

log "Generating HTML report into: ${COVERAGE_DIR}"
rm -rf "${COVERAGE_DIR}"
mkdir -p "${COVERAGE_DIR}"
genhtml -o "${COVERAGE_DIR}" "${BUILD_DIR}/coverage.info"

log_section "Done"
log "Coverage report generated:"
echo "${COVERAGE_DIR}/index.html"
