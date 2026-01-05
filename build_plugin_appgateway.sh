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
ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# AppGateway plugin sources are upstream-synced and live in this workspace at:
#   app-gateway2/plugin/AppGateway
# They MUST be treated as read-only (see plugin/AppGateway/IMMUTABLE_UPSTREAM.md).
PLUGIN_SRC="${ROOT}/plugin/AppGateway"
INSTALL_PREFIX="${ROOT}/dependencies/install"
BUILD_DIR="${ROOT}/build/plugin_appgateway"

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
# WPEFrameworkPluginsConfig.cmake depends on find_package(ConfigGenerator) but the SDK
# provides this as a module, not a package config.
THUNDER_MODULES_DIR="${INSTALL_PREFIX}/include/WPEFramework/Modules"
if [[ -d "${THUNDER_MODULES_DIR}" ]]; then
  log "Thunder CMake modules dir: ${THUNDER_MODULES_DIR}"
else
  die "Thunder CMake modules dir not found: ${THUNDER_MODULES_DIR}"
fi

# Some AppGateway sources depend on helper headers (StringUtils, ErrorUtils, WsManager,
# UtilsJsonrpcDirectLink, etc.). We vendor upstream helper trees under app-gateway2/helpers/.
# Include BOTH so the compiler sees consistent definitions for all plugin sources.
RDKSERVICES_HELPERS_DIR="${ROOT}/app-gateway2/helpers/rdkservices-comcast/helpers"
ENTSERVICES_HELPERS_DIR="${ROOT}/app-gateway2/helpers/entservices-infra/helpers"

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

# Build up a single, consistent flags string for CMake.
# Note: we intentionally append to existing CMAKE_CXX_FLAGS, if any are passed in env/toolchain.
HELPER_CXX_FLAGS=""
if [[ ${#CXX_HELPER_INCLUDES[@]} -gt 0 ]]; then
  HELPER_CXX_FLAGS="$(printf "%s " "${CXX_HELPER_INCLUDES[@]}")"
fi

# Configure/build/install
cmake -G Ninja -S "${PLUGIN_SRC}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}" \
  -DCMAKE_MODULE_PATH="${THUNDER_MODULES_DIR}" \
  ${HELPER_CXX_FLAGS:+-DCMAKE_CXX_FLAGS="${CMAKE_CXX_FLAGS:-} ${HELPER_CXX_FLAGS}"}

cmake --build "${BUILD_DIR}" --target install

# Verify install location and create compatibility symlink if needed.
PLUGIN_SO="${INSTALL_PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so"
COMPAT_DIR="${INSTALL_PREFIX}/lib/plugins"
COMPAT_SO="${COMPAT_DIR}/libWPEFrameworkAppGateway.so"

if [[ -f "${PLUGIN_SO}" ]]; then
  log "Installed plugin: ${PLUGIN_SO}"
else
  # Some installations may use a different STORAGE_DIRECTORY; search for it.
  FOUND="$(find "${INSTALL_PREFIX}/lib" -maxdepth 5 -type f -name 'libWPEFrameworkAppGateway.so' 2>/dev/null | head -n 1 || true)"
  if [[ -n "${FOUND}" ]]; then
    PLUGIN_SO="${FOUND}"
    log "Installed plugin found at: ${PLUGIN_SO}"
  else
    die "Installed plugin not found under ${INSTALL_PREFIX}/lib (expected libWPEFrameworkAppGateway.so)."
  fi
fi

mkdir -p "${COMPAT_DIR}"
ln -sf "${PLUGIN_SO}" "${COMPAT_SO}"
log "Compat symlink:"
log "  ${COMPAT_SO} -> ${PLUGIN_SO}"

log "Done."
