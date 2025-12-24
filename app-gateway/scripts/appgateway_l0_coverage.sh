#!/usr/bin/env bash
set -euo pipefail

# Generates an lcov + genhtml report for the AppGateway l0test build.
#
# Usage (from repo root):
#   bash app-gateway2/app-gateway/scripts/appgateway_l0_coverage.sh build/appgatewayl0test
#
# If lcov is missing:
#   sudo apt install -y lcov

BUILD_DIR="${1:-build/appgatewayl0test}"

if [[ ! -d "${BUILD_DIR}" ]]; then
  echo "Build directory not found: ${BUILD_DIR}" >&2
  echo "Hint: configure & build first, e.g.:" >&2
  echo "  cmake -G Ninja -S app-gateway2/app-gateway -B ${BUILD_DIR} -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=\$PWD/dependencies/install -DCMAKE_INSTALL_PREFIX=\$PWD/dependencies/install" >&2
  echo "  cmake --build ${BUILD_DIR} --target appgateway_l0test -j" >&2
  exit 2
fi

lcov -c -o "${BUILD_DIR}/coverage.info" -d "${BUILD_DIR}"
genhtml -o "${BUILD_DIR}/coverage" "${BUILD_DIR}/coverage.info"

echo "Coverage report generated:"
echo "  ${BUILD_DIR}/coverage/index.html"
