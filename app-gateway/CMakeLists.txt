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

# Enable CTest/ctest integration for this project
enable_testing()

# Thunder typically defines the NAMESPACE cache variable as WPEFramework
if (NOT NAMESPACE)
    set(NAMESPACE WPEFramework)
endif()

# Manually point to local Thunder (WPEFramework) SDK install instead of using find_package.
# This path is relative to this CMakeLists:
#   ../dependencies/install
# which should contain:
#   include/WPEFramework/...
#   lib/libWPEFrameworkCore.so, libWPEFrameworkPlugins.so, etc.
# Resolve Thunder (WPEFramework) SDK paths and normalize them to absolute paths to avoid
# GCC issues with relative segments in include directories.
set(THUNDER_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../dependencies/install" CACHE PATH "Path to Thunder (WPEFramework) install prefix" FORCE)
get_filename_component(THUNDER_INSTALL_PATH_ABS "${THUNDER_INSTALL_PATH}" REALPATH)

set(THUNDER_INCLUDE_DIR "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
set(THUNDER_LIBRARY_DIR "${THUNDER_INSTALL_PATH_ABS}/lib")

# Locate Thunder (WPEFramework) libraries explicitly to ensure the linker finds them
find_library(THUNDER_CORE_LIB NAMES WPEFrameworkCore HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)
find_library(THUNDER_PLUGINS_LIB NAMES WPEFrameworkPlugins HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)

if (NOT THUNDER_CORE_LIB OR NOT THUNDER_PLUGINS_LIB)
    message(FATAL_ERROR "WPEFramework libraries not found in ${THUNDER_LIBRARY_DIR}. Please ensure dependencies are installed.")
endif()

# ========== Build AppGateway plugin ==========
set(TARGET_GATEWAY ${NAMESPACE}AppGateway)

add_library(${TARGET_GATEWAY} SHARED
    AppGateway/Module.cpp
    AppGateway/AppGateway.cpp
    AppGateway/Resolver.cpp
    AppGateway/ResolutionStore.cpp
    AppGateway/RequestRouter.cpp
    AppGateway/ConnectionRegistry.cpp
    AppGateway/GatewayWebSocket.cpp
    AppGateway/GatewayClient.cpp
)

target_include_directories(${TARGET_GATEWAY}
    PUBLIC
        "${THUNDER_INCLUDE_DIR}"
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/AppGateway>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

# Use C++11
target_compile_features(${TARGET_GATEWAY} PRIVATE cxx_std_11)

# Define Thunder namespace macro for builds that don't inject it from the superproject.
if (NOT DEFINED NAMESPACE)
    set(NAMESPACE WPEFramework)
endif()
target_compile_definitions(${TARGET_GATEWAY} PRIVATE NAMESPACE=${NAMESPACE})

target_link_libraries(${TARGET_GATEWAY}
    PRIVATE
        ${THUNDER_CORE_LIB}
        ${THUNDER_PLUGINS_LIB}
)

set_target_properties(${TARGET_GATEWAY}
    PROPERTIES
        FRAMEWORK FALSE
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        OUTPUT_NAME "AppGateway"
)

# Install AppGateway shared object and config
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

install(TARGETS ${TARGET_GATEWAY}
    LIBRARY DESTINATION lib/${NAMESPACE_LIB}/plugins COMPONENT libs
)

install(FILES
    AppGateway/AppGateway.conf.in
    AppGateway/AppGateway.config
    DESTINATION etc/${NAMESPACE_LIB}/plugins
)

# ========== Build App2AppProvider plugin ==========
set(TARGET_A2AP ${NAMESPACE}App2AppProvider)

add_library(${TARGET_A2AP} SHARED
    App2AppProvider/Module.cpp
    App2AppProvider/App2AppProvider.cpp
    App2AppProvider/App2AppProvider.h
    App2AppProvider/App2AppProviderImplementation.cpp
    App2AppProvider/App2AppProviderImplementation.h
)

# Include WPEFramework SDK, plugin sources, AppGateway module header (for "Module.h" include),
# and the repository root for IApp2AppProvider.h, IAppGateway.h, UtilsLogging.h.
target_include_directories(${TARGET_A2AP}
    PUBLIC
        "${THUNDER_INCLUDE_DIR}"
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/App2AppProvider>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/AppGateway>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../..>
)

target_compile_features(${TARGET_A2AP} PRIVATE cxx_std_11)
target_compile_definitions(${TARGET_A2AP} PRIVATE NAMESPACE=${NAMESPACE})

target_link_libraries(${TARGET_A2AP}
    PRIVATE
        ${THUNDER_CORE_LIB}
        ${THUNDER_PLUGINS_LIB}
)

set_target_properties(${TARGET_A2AP}
    PROPERTIES
        FRAMEWORK FALSE
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        OUTPUT_NAME "App2AppProvider"
)

install(TARGETS ${TARGET_A2AP}
    LIBRARY DESTINATION lib/${NAMESPACE_LIB}/plugins COMPONENT libs
)

# Build unit tests for gateway (existing)
add_subdirectory(tests)
