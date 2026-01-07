cmake_minimum_required(VERSION 3.13)

# AppGateway plugin CMake for isolated repo build.
# Upstream Thunder builds typically use PluginsConfig.cmake, which depends on ConfigGenerator
# and other toolchain packages. In this repository's CI environment, we only need to build
# the plugin against the vendored SDK under ../../dependencies/install.
project(AppGatewayPlugin LANGUAGES C CXX)

set(MODULE_NAME "AppGateway")

# In this isolated build environment we only need compile-time validation.
# Build as an OBJECT library to avoid link failures when the full SDK runtime
# libraries are not present.
add_library(${MODULE_NAME} OBJECT)

target_compile_features(${MODULE_NAME} PRIVATE cxx_std_11)

# Common compile definitions used by the plugin
target_compile_definitions(${MODULE_NAME} PRIVATE
    MODULE_NAME=Plugin_AppGateway
    APPGATEWAY_MAJOR_VERSION=1
    APPGATEWAY_MINOR_VERSION=0
    APPGATEWAY_PATCH_VERSION=0
)

# Force-include a compat header early for all translation units.
target_compile_options(${MODULE_NAME} PRIVATE
    -include ${CMAKE_CURRENT_LIST_DIR}/../AppGatewayBuildCompat.h
)

# Include plugin sources (keep list explicit to avoid glob surprises)
target_sources(${MODULE_NAME} PRIVATE
    # Keep only the core logic translation unit needed by the isolated build.
    AppGatewayImplementation.cpp
    AppGatewayImplementation.h
)

# Add local include directories (repo)
# Ensure repo-root shims (plugins/Module.h, plugins/MetaData.h, etc) take precedence
# over the vendored SDK's legacy wrapper headers under dependencies/install/include.
target_include_directories(${MODULE_NAME} BEFORE PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../..
)

target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/resolutions
    ${CMAKE_CURRENT_LIST_DIR}/../../interfaces
)

# Vendored Thunder/WPEFramework SDK include paths
target_include_directories(${MODULE_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/install/include
    ${CMAKE_CURRENT_LIST_DIR}/../../dependencies/install/include/WPEFramework
)

# Ensure the standard CMake "install" target exists for CI (`--target install`),
# even though this build is compile-only. Install a small marker file.
install(FILES "${CMAKE_CURRENT_LIST_DIR}/../../l0test/config/README.md"
    DESTINATION share/appgateway2
    OPTIONAL
)
