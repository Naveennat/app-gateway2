# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2025
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

project(AppGateway LANGUAGES CXX)

# Thunder typically defines the NAMESPACE cache variable as WPEFramework
if (NOT NAMESPACE)
    set(NAMESPACE WPEFramework)
endif()

# Target name follows Thunder convention
set(TARGET ${NAMESPACE}AppGateway)

# Manually point to local Thunder (WPEFramework) SDK install instead of using find_package.
# This path is relative to this CMakeLists:
#   ../../dependencies/install
# which should contain:
#   include/WPEFramework/...
#   lib/libWPEFrameworkCore.so, libWPEFrameworkPlugins.so, etc.
# Resolve Thunder (WPEFramework) SDK paths and normalize them to absolute paths to avoid
# GCC issues with relative segments in include directories.
set(THUNDER_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/install" CACHE PATH "Path to Thunder (WPEFramework) install prefix")
get_filename_component(THUNDER_INSTALL_PATH_ABS "${THUNDER_INSTALL_PATH}" REALPATH)

set(THUNDER_INCLUDE_DIR "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
set(THUNDER_INCLUDE_ROOT "${THUNDER_INSTALL_PATH_ABS}/include")
set(THUNDER_LIBRARY_DIR "${THUNDER_INSTALL_PATH_ABS}/lib")

# Library sources
# NOTE: Intentionally do NOT include a PermissionManager.cpp as the implementation is header-only here.
add_library(${TARGET} SHARED
    AppGateway/Module.cpp
    AppGateway/AppGateway.cpp
    AppGateway/Resolver.cpp
    AppGateway/ResolutionStore.cpp
    AppGateway/RequestRouter.cpp
    AppGateway/ConnectionRegistry.cpp
    AppGateway/GatewayWebSocket.cpp
    AppGateway/GatewayClient.cpp
)

# Includes:
# - Local project includes
# - Thunder headers from the manual SDK path
# Ensure the root include path (install/include) is first so angle includes like <core/core.h>
# resolve correctly to ${THUNDER_INSTALL_PATH_ABS}/include/WPEFramework/core/core.h.
target_include_directories(${TARGET}
    PRIVATE
        "${THUNDER_INCLUDE_ROOT}"
        "${THUNDER_INCLUDE_DIR}"
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/AppGateway>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

# Use C++11
target_compile_features(${TARGET} PRIVATE cxx_std_11)

# Define Thunder namespace macro for builds that don't inject it from the superproject.
# Many Thunder headers reference the NAMESPACE (default: WPEFramework).
if (NOT DEFINED NAMESPACE)
    set(NAMESPACE WPEFramework)
endif()
target_compile_definitions(${TARGET} PRIVATE NAMESPACE=${NAMESPACE})

# Link against Thunder libs from the manual SDK path
link_directories("${THUNDER_LIBRARY_DIR}")

target_link_libraries(${TARGET}
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkPlugins
)

set_target_properties(${TARGET}
    PROPERTIES
        FRAMEWORK FALSE
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        OUTPUT_NAME "AppGateway"
)

# Install shared object to plugins lib dir and config to etc dir (convention)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib/${NAMESPACE_LIB}/plugins COMPONENT libs
)

install(FILES
    AppGateway/AppGateway.conf.in
    AppGateway/AppGateway.config
    DESTINATION etc/${NAMESPACE_LIB}/plugins
)
