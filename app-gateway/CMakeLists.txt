cmake_minimum_required(VERSION 3.13)

# AppGateway top-level build (isolated repo build).
# This repository vendors a prebuilt Thunder/WPEFramework SDK under ../dependencies/install.
# Ensure both CMake package discovery and header include resolution work in this standalone build.
project(AppGateway VERSION 1.0.0)

# Allow consumers/invokers to override, but default to the vendored SDK location.
set(APPGATEWAY_SDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/../dependencies/install" CACHE PATH
    "Path to vendored Thunder/WPEFramework SDK install prefix")

# Make sure find_package() and WPEFramework cmake config files are discoverable if used by subdirs.
list(APPEND CMAKE_PREFIX_PATH "${APPGATEWAY_SDK_ROOT}")

# Ensure common legacy include layouts resolve:
#  - <core/...>, <plugins/...>, <websocket/...>, <interfaces/...>
#  - <WPEFramework/core/...> etc.
include_directories(
    "${APPGATEWAY_SDK_ROOT}/include"
    "${APPGATEWAY_SDK_ROOT}/include/WPEFramework"
)

# Provide write_config() for isolated builds (upstream supplies this via FindConfigGenerator.cmake).
# The plugin's CMakeLists (branch ground-truth) calls write_config(...).
include(${CMAKE_CURRENT_LIST_DIR}/cmake/WriteConfigCompat.cmake)

# Build the actual plugin sources (kept under ../plugin/AppGateway in this repo).
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/../plugin/AppGateway AppGateway)
