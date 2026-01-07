#!/usr/bin/env bash
#
# AppGateway L0 tests + coverage (new repo layout)
# -----------------------------------------------
# Layout assumptions:
#   - Plugin sources:        app-gateway2/plugin/AppGateway
#   - L0 tests (sources):    app-gateway2/tests/l0/appgateway/l0test
#   - Build output:          app-gateway2/build/tests_l0_appgateway
#   - Thunder SDK prefix:    app-gateway2/dependencies/install
#   - Plugin install dir:    dependencies/install/lib/wpeframework/plugins (preferred)
#                            dependencies/install/lib/plugins            (compat)
#
# Notes:
# - Does NOT modify any non-test .cpp/.h.
# - Builds in a separate build directory (requested).
# - Generates lcov+genhtml output under tests/l0/appgateway/coverage/
#
set -euo pipefail

# --- FORCE USE OF THUNDER/WPEFRAMEWORK LIBS ---
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"  # -> app-gateway2

DEPS_LIB="${ROOT}/dependencies/install/lib"
PLUGINS_LIB="${DEPS_LIB}/plugins"
WPE_PLUGINS_LIB="${DEPS_LIB}/wpeframework/plugins"

export LD_LIBRARY_PATH="${DEPS_LIB}:${PLUGINS_LIB}:${WPE_PLUGINS_LIB}:${LD_LIBRARY_PATH:-}"

log()        { echo "[run_coverage] $*"; }
log_section(){ echo ""; echo "[run_coverage] ===== $* ====="; }
warn()       { echo "[run_coverage][WARN] $*" >&2; }
die()        { echo "[run_coverage][ERROR] $*" >&2; exit 1; }

need_cmd() { command -v "$1" >/dev/null 2>&1; }

TEST_SRC_DIR="${ROOT}/tests/l0/appgateway/l0test"
BUILD_DIR="${ROOT}/build/tests_l0_appgateway"
COVERAGE_DIR="${ROOT}/tests/l0/appgateway/coverage"
INSTALL_PREFIX="${ROOT}/dependencies/install"

# Plugin search paths (requested)
PLUGIN_SO_WPE="${INSTALL_PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so"
PLUGIN_SO_COMPAT="${INSTALL_PREFIX}/lib/plugins/libWPEFrameworkAppGateway.so"

DEFAULT_RESOLUTIONS_PATH="${ROOT}/plugin/AppGateway/resolutions/resolution.base.json"

log "ROOT=${ROOT}"
log "TEST_SRC_DIR=${TEST_SRC_DIR}"
log "BUILD_DIR=${BUILD_DIR}"
log "INSTALL_PREFIX=${INSTALL_PREFIX}"
log "COVERAGE_DIR=${COVERAGE_DIR}"

log_section "Tool checks"
need_cmd cmake   || die "cmake not found in PATH."
need_cmd ninja   || die "ninja not found in PATH."
need_cmd lcov    || die "lcov not found in PATH."
need_cmd genhtml || die "genhtml not found in PATH."

log_section "Ensure runtime test configs"
if [[ -x "${ROOT}/scripts/ensure_test_configs.sh" ]]; then
  "${ROOT}/scripts/ensure_test_configs.sh"
else
  warn "Missing ${ROOT}/scripts/ensure_test_configs.sh; continuing without installing /etc/app-gateway fallback config."
fi

log_section "Input checks"
[[ -d "${TEST_SRC_DIR}" ]] || die "L0 test directory not found: ${TEST_SRC_DIR}"
[[ -f "${TEST_SRC_DIR}/CMakeLists.txt" ]] || die "CMakeLists.txt not found in: ${TEST_SRC_DIR}"
[[ -d "${INSTALL_PREFIX}" ]] || die "Install prefix not found: ${INSTALL_PREFIX} (expected Thunder SDK already installed)"

RESOLUTIONS_PATH="${APPGATEWAY_RESOLUTIONS_PATH:-${DEFAULT_RESOLUTIONS_PATH}}"
[[ -f "${RESOLUTIONS_PATH}" ]] || die "Resolution base file not found: ${RESOLUTIONS_PATH}"

log_section "Build AppGateway plugin (ensure up-to-date)"
# The L0 harness links against the installed AppGateway plugin .so under dependencies/install.
# If the plugin isn't rebuilt here, tests can run against a stale binary even when repo sources changed.
# If the user explicitly provides APPGATEWAY_PLUGIN_SO, assume they manage the binary and skip rebuilding.
if [[ -z "${APPGATEWAY_PLUGIN_SO:-}" ]]; then
  if [[ -x "${ROOT}/scripts/build_plugin_appgateway.sh" ]]; then
    "${ROOT}/scripts/build_plugin_appgateway.sh"
  elif [[ -x "${ROOT}/build_plugin_appgateway.sh" ]]; then
    "${ROOT}/build_plugin_appgateway.sh"
  else
    warn "No build_plugin_appgateway.sh found; continuing without rebuilding plugin. Tests may use a stale plugin binary."
  fi
else
  log "APPGATEWAY_PLUGIN_SO is set; skipping plugin rebuild and using provided binary: ${APPGATEWAY_PLUGIN_SO}"
fi

APPGATEWAY_PLUGIN_SO="${APPGATEWAY_PLUGIN_SO:-}"
if [[ -z "${APPGATEWAY_PLUGIN_SO}" ]]; then
  if [[ -f "${PLUGIN_SO_WPE}" ]]; then
    APPGATEWAY_PLUGIN_SO="${PLUGIN_SO_WPE}"
  elif [[ -f "${PLUGIN_SO_COMPAT}" ]]; then
    APPGATEWAY_PLUGIN_SO="${PLUGIN_SO_COMPAT}"
  else
    FOUND="$(find "${INSTALL_PREFIX}/lib" -maxdepth 6 -type f -name 'libWPEFrameworkAppGateway.so' 2>/dev/null | head -n 1 || true)"
    if [[ -n "${FOUND}" ]]; then
      APPGATEWAY_PLUGIN_SO="${FOUND}"
    else
      APPGATEWAY_PLUGIN_SO="${PLUGIN_SO_WPE}"
    fi
  fi
fi

if [[ -f "${APPGATEWAY_PLUGIN_SO}" ]]; then
  log "Using AppGateway plugin .so: ${APPGATEWAY_PLUGIN_SO}"
else
  warn "AppGateway plugin .so not found at selected path: ${APPGATEWAY_PLUGIN_SO}"
  warn "Expected one of:"
  warn "  - ${PLUGIN_SO_WPE}"
  warn "  - ${PLUGIN_SO_COMPAT}"
  warn "Continuing; tests may still build but can fail at runtime."
fi

log_section "Coverage flags"
COVERAGE_CXXFLAGS="--coverage -O0 -g"
COVERAGE_LDFLAGS="--coverage"

export CMAKE_PREFIX_PATH="${INSTALL_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

export CXXFLAGS="${COVERAGE_CXXFLAGS}${CXXFLAGS:+ ${CXXFLAGS}}"
export LDFLAGS="${COVERAGE_LDFLAGS}${LDFLAGS:+ ${LDFLAGS}}"

log "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
log "PKG_CONFIG_PATH=${PKG_CONFIG_PATH}"
log "CXXFLAGS=${CXXFLAGS}"
log "LDFLAGS=${LDFLAGS}"

log_section "Clean build directory"
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"
mkdir -p "${COVERAGE_DIR}"

log_section "Prepare Real-plugin link path (optional)"
if [[ -f "${APPGATEWAY_PLUGIN_SO}" ]]; then
  mkdir -p "${ROOT}/build/AppGateway"
  ln -sf "${APPGATEWAY_PLUGIN_SO}" "${ROOT}/build/AppGateway/libWPEFrameworkAppGateway.so"
  log "Symlinked for CMake probe:"
  log "  ${ROOT}/build/AppGateway/libWPEFrameworkAppGateway.so -> ${APPGATEWAY_PLUGIN_SO}"
else
  warn "Skipping plugin symlink (plugin .so missing)."
fi

log_section "Configure (Ninja) and build"
cmake -S "${TEST_SRC_DIR}" -B "${BUILD_DIR}" -G Ninja \
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

cmake --build "${BUILD_DIR}" --target appgateway_l0test -j "${JOBS}"

log_section "Locate test binary"
TEST_BIN=""
for c in "${BUILD_DIR}/appgateway_l0test" "${BUILD_DIR}/AppGateway/appgateway_l0test"; do
  if [[ -x "${c}" ]]; then
    TEST_BIN="${c}"
    break
  fi
done
if [[ -z "${TEST_BIN}" ]]; then
  FOUND="$(find "${BUILD_DIR}" -maxdepth 4 -type f -name appgateway_l0test -perm -u+x 2>/dev/null | head -n 1 || true)"
  [[ -n "${FOUND}" ]] && TEST_BIN="${FOUND}"
fi
[[ -n "${TEST_BIN}" ]] || die "Test binary 'appgateway_l0test' not found after build (searched under ${BUILD_DIR})."

log "Test binary: ${TEST_BIN}"

log_section "Run tests"
PLUGIN_DIR="$(dirname "${APPGATEWAY_PLUGIN_SO}")"

LD_PATH_EXTRA=(
  "${DEPS_LIB}"
  "${PLUGINS_LIB}"
  "${WPE_PLUGINS_LIB}"
  "${BUILD_DIR}"
  "${BUILD_DIR}/AppGateway"
)
if [[ -n "${PLUGIN_DIR}" && -d "${PLUGIN_DIR}" ]]; then
  LD_PATH_EXTRA=( "${PLUGIN_DIR}" "${LD_PATH_EXTRA[@]}" )
fi
LD_LIBRARY_PATH_COMPOSED="$(IFS=:; echo "${LD_PATH_EXTRA[*]}")"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH_COMPOSED}:${LD_LIBRARY_PATH}"

APPGATEWAY_RESOLUTIONS_PATH="${RESOLUTIONS_PATH}" \
APPGATEWAY_PLUGIN_SO="${APPGATEWAY_PLUGIN_SO}" \
APPGATEWAY_L0_DISABLE_COMRPC=1 \
"${TEST_BIN}"

log_section "Generate coverage"
INFO_FILE="${COVERAGE_DIR}/coverage.info"
HTML_DIR="${COVERAGE_DIR}/html"

lcov -c -o "${INFO_FILE}" -d "${BUILD_DIR}" --ignore-errors empty
rm -rf "${HTML_DIR}"
mkdir -p "${HTML_DIR}"
genhtml -o "${HTML_DIR}" "${INFO_FILE}"

log_section "Done"
echo "${HTML_DIR}/index.html"
