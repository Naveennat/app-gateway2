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
BUILD_DIR_BASE="${ROOT}/build/plugin_appgateway_${TS}"

# Required log locations
COVERAGE_DIR="${ROOT}/tests/l0/appgateway/coverage"
BUILD_LOG_DIR="${ROOT}/build_logs"
CONFIGURE_LOG="${COVERAGE_DIR}/build_plugin_appgateway_configure.log"
BUILD_LOG="${COVERAGE_DIR}/build_plugin_appgateway_build.log"
APPEND_LOG="${BUILD_LOG_DIR}/build_plugin_appgateway_${TS}.log"
RESULT_FILE="${BUILD_LOG_DIR}/RESULT_plugin_appgateway_${TS}.txt"

mkdir -p "${COVERAGE_DIR}" "${BUILD_LOG_DIR}"

log "SCRIPT_DIR=${SCRIPT_DIR}"
log "ROOT=${ROOT}"
log "PLUGIN_SRC=${PLUGIN_SRC}"
log "SDK_PREFIX=${SDK_PREFIX}"
log "BUILD_DIR_BASE=${BUILD_DIR_BASE}"
log "CONFIGURE_LOG=${CONFIGURE_LOG}"
log "BUILD_LOG=${BUILD_LOG}"
log "APPEND_LOG=${APPEND_LOG}"
log "RESULT_FILE=${RESULT_FILE}"

[[ -d "${PLUGIN_SRC}" ]] || die "Plugin source directory not found: ${PLUGIN_SRC}"
[[ -f "${PLUGIN_SRC}/CMakeLists.txt" ]] || die "CMakeLists.txt not found in: ${PLUGIN_SRC}"
[[ -d "${SDK_PREFIX}" ]] || die "SDK prefix not found (expected Thunder SDK already installed): ${SDK_PREFIX}"

# Ensure CMake and pkg-config prefer the Thunder 4.4 SDK in dependencies/install.
# Note: Thunder's package config files live under ${SDK_PREFIX}/lib/cmake/**, so include
# that in CMAKE_PREFIX_PATH for reliable find_package() behavior.
export CMAKE_PREFIX_PATH="${SDK_PREFIX}:${SDK_PREFIX}/lib/cmake${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"
export PKG_CONFIG_PATH="${SDK_PREFIX}/lib/pkgconfig${PKG_CONFIG_PATH:+:${PKG_CONFIG_PATH}}"

log "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"
log "PKG_CONFIG_PATH=${PKG_CONFIG_PATH}"

# Thunder provides some dependencies as CMake modules (e.g., FindConfigGenerator.cmake).
THUNDER_MODULES_DIR="${SDK_PREFIX}/include/WPEFramework/Modules"
if [[ -d "${THUNDER_MODULES_DIR}" ]]; then
  log "Thunder CMake modules dir: ${THUNDER_MODULES_DIR}"
else
  die "Thunder CMake modules dir not found: ${THUNDER_MODULES_DIR}"
fi

# Some AppGateway sources depend on helper headers.
# The plugin includes these by short name:
#   - UtilsLogging.h
#   - WsManager.h
#
# We inject include dirs into the plugin CMake using APPGATEWAY_EXTRA_INCLUDE_DIRS
# as a CMake LIST in the REQUIRED priority order:
#   1) ${ROOT}/helpers/rdkservices-comcast/helpers
#   2) ${ROOT}/helpers/entservices-infra/helpers
#   3) ${ROOT}/helpers
#   4) ${ROOT}/interfaces
#   5) ${ROOT}/supporting_files
APPGATEWAY_EXTRA_INCLUDE_DIRS_ORDERED=(
  "${ROOT}/helpers/rdkservices-comcast/helpers"
  "${ROOT}/helpers/entservices-infra/helpers"
  "${ROOT}/helpers"
  "${ROOT}/interfaces"
  "${ROOT}/supporting_files"
)

APPGATEWAY_EXTRA_INCLUDE_DIRS_RESOLVED=()

add_inc_if_dir() {
  local d="$1"
  if [[ -d "$d" ]]; then
    APPGATEWAY_EXTRA_INCLUDE_DIRS_RESOLVED+=("$d")
  else
    log "Extra include not found (continuing): $d"
  fi
}

for d in "${APPGATEWAY_EXTRA_INCLUDE_DIRS_ORDERED[@]}"; do
  add_inc_if_dir "$d"
done

# Echo resolved paths BEFORE configure (as requested)
log "Resolved APPGATEWAY_EXTRA_INCLUDE_DIRS (priority order):"
if [[ ${#APPGATEWAY_EXTRA_INCLUDE_DIRS_RESOLVED[@]} -eq 0 ]]; then
  log "  (none)"
else
  for d in "${APPGATEWAY_EXTRA_INCLUDE_DIRS_RESOLVED[@]}"; do
    log "  $d"
  done
fi

# Convert to a semicolon-separated CMake LIST.
# NOTE: must NOT use a subshell here, otherwise the assignment won't persist.
APPGATEWAY_EXTRA_INCLUDE_LIST=""
if [[ ${#APPGATEWAY_EXTRA_INCLUDE_DIRS_RESOLVED[@]} -gt 0 ]]; then
  IFS=';' APPGATEWAY_EXTRA_INCLUDE_LIST="${APPGATEWAY_EXTRA_INCLUDE_DIRS_RESOLVED[*]}"
  unset IFS
fi

# Try standards in order: C++11 -> C++14 -> C++17 (stop at first success).
# IMPORTANT: Do not inject any compiler -std flags here; we only pass CMake cache vars.
CXX_STANDARDS_TO_TRY=(11 14 17)
SUCCESS=0
LAST_STD=""

for STD in "${CXX_STANDARDS_TO_TRY[@]}"; do
  BUILD_DIR="${BUILD_DIR_BASE}_cxx${STD}"
  LAST_STD="${STD}"

  # Configure (capture to coverage log and append log)
  {
    log "=== CONFIGURE (timestamp ${TS}) (C++${STD}) ==="
    log "BUILD_DIR=${BUILD_DIR}"
    log "APPGATEWAY_EXTRA_INCLUDE_DIRS=${APPGATEWAY_EXTRA_INCLUDE_LIST}"
    cmake -G Ninja -S "${PLUGIN_SRC}" -B "${BUILD_DIR}" \
      -DCMAKE_BUILD_TYPE=Debug \
      -DNAMESPACE=WPEFramework \
      -DCMAKE_INSTALL_PREFIX="${SDK_PREFIX}" \
      -DCMAKE_INSTALL_SYSCONFDIR="etc" \
      -DAPPGATEWAY_SYSCONFDIR="etc/app-gateway" \
      -DCMAKE_PREFIX_PATH="${CMAKE_PREFIX_PATH}" \
      -DCMAKE_MODULE_PATH="${THUNDER_MODULES_DIR}" \
      -DAPPGATEWAY_EXTRA_INCLUDE_DIRS:STRING="${APPGATEWAY_EXTRA_INCLUDE_LIST}" \
      -DCMAKE_CXX_STANDARD="${STD}" \
      -DCMAKE_CXX_STANDARD_REQUIRED=ON \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

    # After configure, print the exact CMakeCache entry for verification (as requested).
    if [[ -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
      log "CMakeCache.txt entry:"
      grep -n '^APPGATEWAY_EXTRA_INCLUDE_DIRS:' "${BUILD_DIR}/CMakeCache.txt" || true
      grep -n '^CMAKE_CXX_STANDARD:' "${BUILD_DIR}/CMakeCache.txt" || true
      grep -n '^CMAKE_CXX_STANDARD_REQUIRED:' "${BUILD_DIR}/CMakeCache.txt" || true
    else
      log "CMakeCache.txt not found at ${BUILD_DIR}/CMakeCache.txt"
    fi
  } 2>&1 | tee "${CONFIGURE_LOG}" | tee -a "${APPEND_LOG}"

  # Build + install (capture to coverage log and append log)
  if {
    log "=== BUILD+INSTALL (timestamp ${TS}) (C++${STD}) ==="
    cmake --build "${BUILD_DIR}" --target install
  } 2>&1 | tee "${BUILD_LOG}" | tee -a "${APPEND_LOG}"; then
    SUCCESS=1
    break
  else
    log "Build failed for C++${STD}, will retry with next standard (if any)."
  fi
done

if [[ "${SUCCESS}" -ne 1 ]]; then
  {
    echo "RESULT: FAIL"
    echo "Timestamp: ${TS}"
    echo "Last tried C++ standard: C++${LAST_STD}"
    echo "See logs:"
    echo "  ${CONFIGURE_LOG}"
    echo "  ${BUILD_LOG}"
    echo "  ${APPEND_LOG}"
  } > "${RESULT_FILE}"
  die "Build failed for all tried standards: ${CXX_STANDARDS_TO_TRY[*]}"
fi

{
  echo "RESULT: OK"
  echo "Timestamp: ${TS}"
  echo "C++ standard used: C++${LAST_STD}"
  echo "Build dir: ${BUILD_DIR}"
  echo "See logs:"
  echo "  ${CONFIGURE_LOG}"
  echo "  ${BUILD_LOG}"
  echo "  ${APPEND_LOG}"
} > "${RESULT_FILE}"

# Verify install location and create compatibility symlink if needed.
#
# In some SDK builds STORAGE_DIRECTORY may be unset/empty, causing the plugin to
# install directly to ${SDK_PREFIX}/lib/plugins. In that case, the "compat"
# symlink path equals the real path, and `ln -sf` would error with "same file".
PLUGIN_SO_PREFERRED="${SDK_PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so"
COMPAT_DIR="${SDK_PREFIX}/lib/plugins"
COMPAT_SO="${COMPAT_DIR}/libWPEFrameworkAppGateway.so"

PLUGIN_SO=""

if [[ -f "${PLUGIN_SO_PREFERRED}" ]]; then
  PLUGIN_SO="${PLUGIN_SO_PREFERRED}"
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

# Only create/update compat symlink if it would not point to the same path.
if [[ "$(readlink -f "${PLUGIN_SO}")" == "$(readlink -f "${COMPAT_SO}" 2>/dev/null || true)" ]] || [[ "$(readlink -f "${PLUGIN_SO}")" == "$(readlink -f "${COMPAT_DIR}/libWPEFrameworkAppGateway.so" 2>/dev/null || true)" ]]; then
  log "Compat symlink already satisfied (same target/path):"
  log "  ${COMPAT_SO}"
elif [[ "${PLUGIN_SO}" == "${COMPAT_SO}" ]]; then
  log "Compat symlink not required (plugin already at compat path):"
  log "  ${COMPAT_SO}"
else
  ln -sf "${PLUGIN_SO}" "${COMPAT_SO}"
  log "Compat symlink:"
  log "  ${COMPAT_SO} -> ${PLUGIN_SO}"
fi

log "Done."
