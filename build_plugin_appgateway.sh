#!/usr/bin/env bash
#
# Build + install AppGateway plugin into dependencies/install using Thunder 4.4 SDK.
#
# This script is intentionally self-contained and derives paths from its own location.
# It does not modify plugin sources; it only configures/builds/installs.
#
# Output:
#  - build tree:        build/plugin_appgateway
#  - install prefix:    dependencies/install
#  - plugin installed:  dependencies/install/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so
#  - compat symlink:    dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so (if needed)
#
set -euo pipefail

log()  { echo "[build_plugin_appgateway] $*"; }
die()  { echo "[build_plugin_appgateway][ERROR] $*" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Repository/container root is the directory where this script lives.
# (Requirements: derive ROOT via the script location.)
ROOT="$(cd "${SCRIPT_DIR}" && pwd)"

# Immutable upstream plugin sources.
PLUGIN_SRC="${ROOT}/plugin/AppGateway"

# Thunder 4.4 SDK prefix (preinstalled under this repo).
SDK_PREFIX="${ROOT}/dependencies/install"

# Use a timestamped build dir to avoid needing to delete older build trees.
TS="$(date +%Y%m%d_%H%M%S)"
BUILD_DIR="${ROOT}/build/plugin_appgateway_${TS}"

# Required log locations
COVERAGE_DIR="${ROOT}/tests/l0/appgateway/coverage"
BUILD_LOG_DIR="${ROOT}/build_logs"
CONFIGURE_LOG="${COVERAGE_DIR}/build_plugin_appgateway_configure.log"
BUILD_LOG="${COVERAGE_DIR}/build_plugin_appgateway_build.log"
APPEND_LOG="${BUILD_LOG_DIR}/build_plugin_appgateway_${TS}.log"

mkdir -p "${COVERAGE_DIR}" "${BUILD_LOG_DIR}"

log "SCRIPT_DIR=${SCRIPT_DIR}"
log "ROOT=${ROOT}"
log "PLUGIN_SRC=${PLUGIN_SRC}"
log "SDK_PREFIX=${SDK_PREFIX}"
log "BUILD_DIR=${BUILD_DIR}"
log "CONFIGURE_LOG=${CONFIGURE_LOG}"
log "BUILD_LOG=${BUILD_LOG}"
log "APPEND_LOG=${APPEND_LOG}"

[[ -d "${PLUGIN_SRC}" ]] || die "Plugin source directory not found: ${PLUGIN_SRC}"
[[ -f "${PLUGIN_SRC}/CMakeLists.txt" ]] || die "CMakeLists.txt not found in: ${PLUGIN_SRC}"
[[ -d "${SDK_PREFIX}" ]] || die "SDK prefix not found (expected Thunder SDK already installed): ${SDK_PREFIX}"

# Ensure CMake and pkg-config prefer the Thunder 4.4 SDK in dependencies/install.
# Note: Thunder's package config files live under ${SDK_PREFIX}/lib/cmake/**, so include
# that in CMAKE_PREFIX_PATH as well for reliable find_package() behavior.
export CMAKE_PREFIX_PATH="${SDK_PREFIX}:${SDK_PREFIX}/lib/cmake${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PKG_CONFIG_PATH="${SDK_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

log "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
log "PKG_CONFIG_PATH=${PKG_CONFIG_PATH}"

# Thunder provides some dependencies as CMake modules (e.g., FindConfigGenerator.cmake).
# Include Thunder modules and any helper include dirs already configured previously.
THUNDER_MODULES_DIR="${SDK_PREFIX}/include/WPEFramework/Modules"
if [[ -d "${THUNDER_MODULES_DIR}" ]]; then
  log "Thunder CMake modules dir: ${THUNDER_MODULES_DIR}"
else
  die "Thunder CMake modules dir not found: ${THUNDER_MODULES_DIR}"
fi

# Some AppGateway sources depend on helper headers; include both vendored helper trees.
RDKSERVICES_HELPERS_DIR="${ROOT}/helpers/rdkservices-comcast/helpers"
ENTSERVICES_HELPERS_DIR="${ROOT}/helpers/entservices-infra/helpers"

CXX_HELPER_INCLUDES=()
if [[ -d "${RDKSERVICES_HELPERS_DIR}" ]]; then
  log "Helper include (rdkservices-comcast): ${RDKSERVICES_HELPERS_DIR}"
  CXX_HELPER_INCLUDES+=("-I${RDKSERVICES_HELPERS_DIR}")
else
  log "Helper include not found (continuing): ${RDKSERVICES_HELPERS_DIR}"
fi

if [[ -d "${ENTSERVICES_HELPERS_DIR}" ]]; then
  log "Helper include (entservices-infra): ${ENTSERVICES_HELPERS_DIR}"
  CXX_HELPER_INCLUDES+=("-I${ENTSERVICES_HELPERS_DIR}")
else
  log "Helper include not found (continuing): ${ENTSERVICES_HELPERS_DIR}"
fi

HELPER_CXX_FLAGS=""
if [[ ${#CXX_HELPER_INCLUDES[@]} -gt 0 ]]; then
  HELPER_CXX_FLAGS="$(printf "%s " "${CXX_HELPER_INCLUDES[@]}")"
fi

# Configure (capture to coverage log and append log)
{
  log "=== CONFIGURE (timestamp ${TS}) ==="
  cmake -G Ninja -S "${PLUGIN_SRC}" -B "${BUILD_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_INSTALL_PREFIX="${SDK_PREFIX}" \
    -DCMAKE_PREFIX_PATH="${SDK_PREFIX}" \
    -DCMAKE_MODULE_PATH="${THUNDER_MODULES_DIR}" \
    ${HELPER_CXX_FLAGS:+-DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} ${HELPER_CXX_FLAGS}"}
} 2>&1 | tee "${CONFIGURE_LOG}" | tee -a "${APPEND_LOG}"

# Build + install (capture to coverage log and append log)
{
  log "=== BUILD+INSTALL (timestamp ${TS}) ==="
  cmake --build "${BUILD_DIR}" --target install
} 2>&1 | tee "${BUILD_LOG}" | tee -a "${APPEND_LOG}"

# Verify install location and create compatibility symlink if needed.
PLUGIN_SO="${SDK_PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so"
COMPAT_DIR="${SDK_PREFIX}/lib/plugins"
COMPAT_SO="${COMPAT_DIR}/libWPEFrameworkAppGateway.so"

# Requirement: install libWPEFrameworkAppGateway.so to $SDK_PREFIX/lib/wpeframework/plugins
# (the plugin's CMake install DESTINATION uses lib/${STORAGE_DIRECTORY}/plugins; in our SDK
# STORAGE_DIRECTORY is expected to be 'wpeframework').
if [[ -f "${PLUGIN_SO}" ]]; then
  log "Installed plugin: ${PLUGIN_SO}"
else
  # Fallback: search for it and fail if it wasn't installed at all.
  FOUND="$(find "${SDK_PREFIX}/lib" -maxdepth 5 -type f -name 'libWPEFrameworkAppGateway.so' 2>/dev/null | head -n 1 || true)"
  if [[ -n "${FOUND}" ]]; then
    PLUGIN_SO="${FOUND}"
    log "Installed plugin found at: ${PLUGIN_SO}"
  else
    die "Installed plugin not found under ${SDK_PREFIX}/lib (expected libWPEFrameworkAppGateway.so)."
  fi
fi

mkdir -p "${COMPAT_DIR}"
ln -sf "${PLUGIN_SO}" "${COMPAT_SO}"
log "Compat symlink:"
log "  ${COMPAT_SO} -> ${PLUGIN_SO}"

log "Done."
