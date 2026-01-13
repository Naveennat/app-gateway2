#!/usr/bin/env bash
#
# Build + install AppGateway plugin into app-gateway2/dependencies/install using Thunder 4.4 SDK.
#
# Requirements implemented:
#  - Export/append CMAKE_PREFIX_PATH and PKG_CONFIG_PATH pointing to dependencies/install
#  - Pass -DCMAKE_MODULE_PATH="$PREFIX/include/WPEFramework/Modules" so FindConfigGenerator.cmake resolves
#  - Add include directories for entservices-infra helpers and repo Supporting_Files
#  - Build out-of-source under build/plugin_appgateway and install into dependencies/install
#  - Ensure libWPEFrameworkAppGateway.so is installed under lib/wpeframework/plugins and create/refresh
#    a symlink under lib/plugins
#  - Capture configure/build logs under tests/l0/appgateway/coverage/
#
set -euo pipefail

log()  { echo "[build_plugin_appgateway] $*"; }
die()  { echo "[build_plugin_appgateway][ERROR] $*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

PLUGIN_SRC="${ROOT}/plugin/AppGateway"
INSTALL_PREFIX="${ROOT}/dependencies/install"
BUILD_DIR="${ROOT}/build/plugin_appgateway"

LOG_DIR="${ROOT}/tests/l0/appgateway/coverage"
CONFIG_LOG="${LOG_DIR}/build_plugin_appgateway_configure.log"
BUILD_LOG="${LOG_DIR}/build_plugin_appgateway_build.log"
ERRORS_LATEST="${LOG_DIR}/build_plugin_appgateway_errors.latest.txt"

mkdir -p "${LOG_DIR}"

log "ROOT=${ROOT}"
log "PLUGIN_SRC=${PLUGIN_SRC}"
log "BUILD_DIR=${BUILD_DIR}"
log "INSTALL_PREFIX=${INSTALL_PREFIX}"

[[ -d "${PLUGIN_SRC}" ]] || die "Plugin source directory not found: ${PLUGIN_SRC}"
[[ -f "${PLUGIN_SRC}/CMakeLists.txt" ]] || die "CMakeLists.txt not found in: ${PLUGIN_SRC}"
[[ -d "${INSTALL_PREFIX}" ]] || die "Install prefix not found (expected Thunder SDK already installed): ${INSTALL_PREFIX}"

# Ensure CMake and pkg-config prefer the Thunder 4.4 SDK in dependencies/install.
export CMAKE_PREFIX_PATH="${INSTALL_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PKG_CONFIG_PATH="${INSTALL_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

log "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
log "PKG_CONFIG_PATH=${PKG_CONFIG_PATH}"

# Ensure CMake can locate Thunder's CMake *modules* (FindConfigGenerator.cmake, etc.).
THUNDER_MODULES_DIR="${INSTALL_PREFIX}/include/WPEFramework/Modules"
[[ -d "${THUNDER_MODULES_DIR}" ]] || die "Thunder CMake modules dir not found: ${THUNDER_MODULES_DIR}"
log "Thunder CMake modules dir: ${THUNDER_MODULES_DIR}"

# Helper include dirs (prefer entservices-infra first to avoid local shim conflicts).
ENTSERVICES_HELPERS_DIR="${ROOT}/dependencies/entservices-infra/helpers"
SUPPORTING_FILES_DIR="${ROOT}/Supporting_Files"
LOCAL_HELPERS_DIR="${ROOT}/helpers"

EXTRA_CXX_INCLUDES=()
if [[ -d "${ENTSERVICES_HELPERS_DIR}" ]]; then
  EXTRA_CXX_INCLUDES+=("-I${ENTSERVICES_HELPERS_DIR}")
  log "Extra helper include (preferred): ${ENTSERVICES_HELPERS_DIR}"
else
  log "Extra helper include not found (continuing): ${ENTSERVICES_HELPERS_DIR}"
fi

if [[ -d "${SUPPORTING_FILES_DIR}" ]]; then
  EXTRA_CXX_INCLUDES+=("-I${SUPPORTING_FILES_DIR}")
  log "Extra supporting_files include: ${SUPPORTING_FILES_DIR}"
fi

if [[ -d "${LOCAL_HELPERS_DIR}" ]]; then
  EXTRA_CXX_INCLUDES+=("-I${LOCAL_HELPERS_DIR}")
  log "Extra local helpers include: ${LOCAL_HELPERS_DIR}"
fi

EXTRA_CXX_FLAGS=""
if [[ ${#EXTRA_CXX_INCLUDES[@]} -gt 0 ]]; then
  EXTRA_CXX_FLAGS="${EXTRA_CXX_INCLUDES[*]}"
fi

# Fresh logs
: > "${CONFIG_LOG}"
: > "${BUILD_LOG}"
: > "${ERRORS_LATEST}"

# Configure
# Optional: build plugin with gcov/coverage instrumentation.
# This is used by the L0 coverage runner to include plugin/AppGateway/*.cpp in lcov output.
COVERAGE_CXXFLAGS="${APPGATEWAY_COVERAGE_CXXFLAGS:-}"
COVERAGE_LDFLAGS="${APPGATEWAY_COVERAGE_LDFLAGS:-}"
if [[ "${APPGATEWAY_BUILD_WITH_COVERAGE:-0}" != "0" ]]; then
  [[ -n "${COVERAGE_CXXFLAGS}" ]] || COVERAGE_CXXFLAGS="--coverage -O0 -g"
  [[ -n "${COVERAGE_LDFLAGS}" ]] || COVERAGE_LDFLAGS="--coverage"
  log "Coverage build enabled:"
  log "  COVERAGE_CXXFLAGS=${COVERAGE_CXXFLAGS}"
  log "  COVERAGE_LDFLAGS=${COVERAGE_LDFLAGS}"
fi

COMBINED_CXX_FLAGS=""
if [[ -n "${EXTRA_CXX_FLAGS}" ]]; then
  COMBINED_CXX_FLAGS="${EXTRA_CXX_FLAGS}"
fi
if [[ -n "${COVERAGE_CXXFLAGS}" ]]; then
  COMBINED_CXX_FLAGS="${COMBINED_CXX_FLAGS}${COMBINED_CXX_FLAGS:+ }${COVERAGE_CXXFLAGS}"
fi

log "Configuring..."
(
  set -o pipefail
  cmake -G Ninja -S "${PLUGIN_SRC}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
    -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" \
    -DCMAKE_MODULE_PATH="${THUNDER_MODULES_DIR}" \
    ${COMBINED_CXX_FLAGS:+-DCMAKE_CXX_FLAGS="${COMBINED_CXX_FLAGS}"} \
    ${COVERAGE_LDFLAGS:+-DCMAKE_SHARED_LINKER_FLAGS="${COVERAGE_LDFLAGS}"} \
    ${COVERAGE_LDFLAGS:+-DCMAKE_EXE_LINKER_FLAGS="${COVERAGE_LDFLAGS}"}
) 2>&1 | tee "${CONFIG_LOG}"

# Build + install
log "Building..."
BUILD_RC=0
(
  set -o pipefail
  cmake --build "${BUILD_DIR}" --target install
) 2>&1 | tee "${BUILD_LOG}" || BUILD_RC=$?

if [[ ${BUILD_RC} -ne 0 ]]; then
  log "Build failed; capturing first 50 lines of errors into ${ERRORS_LATEST}"
  # Heuristic: take first 50 error lines from build log if possible, else first 50 lines of full log.
  if grep -n "error:" "${BUILD_LOG}" >/dev/null 2>&1; then
    grep -n "error:" "${BUILD_LOG}" | head -n 50 > "${ERRORS_LATEST}" || true
  else
    head -n 50 "${BUILD_LOG}" > "${ERRORS_LATEST}" || true
  fi
  exit "${BUILD_RC}"
fi

# Verify install location and create compatibility symlink if needed.
PLUGIN_SO="${INSTALL_PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so"
COMPAT_DIR="${INSTALL_PREFIX}/lib/plugins"
COMPAT_SO="${COMPAT_DIR}/libWPEFrameworkAppGateway.so"

if [[ ! -f "${PLUGIN_SO}" ]]; then
  FOUND="$(find "${INSTALL_PREFIX}/lib" -maxdepth 5 -type f -name 'libWPEFrameworkAppGateway.so' 2>/dev/null | head -n 1 || true)"
  [[ -n "${FOUND}" ]] || die "Installed plugin not found under ${INSTALL_PREFIX}/lib (expected libWPEFrameworkAppGateway.so)."
  PLUGIN_SO="${FOUND}"
fi

log "Installed plugin: ${PLUGIN_SO}"
mkdir -p "${COMPAT_DIR}"
ln -sf "${PLUGIN_SO}" "${COMPAT_SO}"
log "Compat symlink:"
log "  ${COMPAT_SO} -> ${PLUGIN_SO}"

log "Done."
