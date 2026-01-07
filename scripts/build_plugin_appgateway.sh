#!/usr/bin/env bash
#
# Build & install the AppGateway plugin against the vendored Thunder/WPEFramework SDK.
#
# This repository layout differs from some upstream tooling:
#   - Plugin sources live under:  app-gateway2/plugin/AppGateway
#   - Vendored SDK prefix:        app-gateway2/dependencies/install
#
# This script is used by automation/CI to ensure the AppGateway plugin is built
# and installed into the local vendored prefix before running L0/L1/L2 tests.
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Repo layout: plugin sources are under app-gateway2/plugin/AppGateway
SRC_DIR="${ROOT}/plugin/AppGateway"

# Build output directory
BUILD_DIR="${ROOT}/build/AppGateway"

# Install prefix for Thunder/WPEFramework + plugins
INSTALL_PREFIX="${ROOT}/dependencies/install"

log() { echo "[build_plugin_appgateway] $*"; }
die() { echo "[build_plugin_appgateway][ERROR] $*" >&2; exit 1; }

[[ -d "${SRC_DIR}" ]] || die "Source directory not found: ${SRC_DIR}"
[[ -f "${SRC_DIR}/CMakeLists.txt" ]] || die "CMakeLists.txt not found under: ${SRC_DIR}"

mkdir -p "${BUILD_DIR}"
mkdir -p "${INSTALL_PREFIX}"

# Ensure CMake can find the vendored SDK packages.
# Many WPEFramework modules export their Config.cmake files under:
#   ${INSTALL_PREFIX}/lib/cmake/WPEFramework*
export CMAKE_PREFIX_PATH="${INSTALL_PREFIX}${CMAKE_PREFIX_PATH:+:${CMAKE_PREFIX_PATH}}"

log "ROOT=${ROOT}"
log "SRC_DIR=${SRC_DIR}"
log "BUILD_DIR=${BUILD_DIR}"
log "INSTALL_PREFIX=${INSTALL_PREFIX}"
log "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}"

# Configure & build
cmake -G Ninja -S "${SRC_DIR}" -B "${BUILD_DIR}" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_INSTALL_PREFIX="${INSTALL_PREFIX}" \
  -DCMAKE_PREFIX_PATH="${INSTALL_PREFIX}"

cmake --build "${BUILD_DIR}" --target install

log "Installed AppGateway plugin into ${INSTALL_PREFIX}"
log "NOTE: Depending on plugin's install rules, the .so may be under:"
log "  - ${INSTALL_PREFIX}/lib/plugins/"
log "  - ${INSTALL_PREFIX}/lib/wpeframework/plugins/ (if present)"
