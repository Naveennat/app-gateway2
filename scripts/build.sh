#!/usr/bin/env sh
# app-gateway2 scripts/build.sh
# Purpose:
# - Ensure the app is build-ready for preview environments.
# - For Node.js: install production dependencies.
# - For C++/CMake: if a top-level CMakeLists.txt exists in this directory, build into ./build.
# Notes:
# - Executable bit hint: this script is intended to be executable (chmod +x), the platform will set perms.

set -eu

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "[build] Starting build steps in ${ROOT_DIR}"

# Node.js dependency install (production-only)
if [ -f package.json ]; then
  echo "[build] Detected package.json; ensuring Node dependencies (production only)..."
  if command -v npm >/dev/null 2>&1; then
    if [ -f package-lock.json ]; then
      echo "[build] Using npm ci --omit=dev (with lockfile)"
      npm ci --omit=dev || npm install --omit=dev
    else
      echo "[build] No package-lock.json; using npm install --omit=dev"
      npm install --omit=dev
    fi
  else
    echo "[build] Warning: npm not found; skipping Node dependency install." >&2
  fi
fi

# C++/CMake build (only if a top-level CMakeLists.txt exists)
if [ -f CMakeLists.txt ]; then
  if command -v cmake >/dev/null 2>&1; then
    echo "[build] Detected top-level CMakeLists.txt; configuring and building into ./build ..."
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    # Build using all available cores or a safe default
    cmake --build build -j || {
      echo "[build] Warning: CMake build failed; continuing. Node server may still run." >&2
    }
  else
    echo "[build] Warning: cmake not available; cannot build C++ target." >&2
  fi
fi

echo "[build] Build steps completed."
