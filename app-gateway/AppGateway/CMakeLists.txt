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

# This subdirectory CMake builds the AppGateway plugin as a shared library.
# It does not use find_package; includes and libraries are referenced directly
# from ../../dependencies/install.

project(AppGateway LANGUAGES CXX)

# Thunder typically defines the NAMESPACE cache variable as WPEFramework.
if (NOT DEFINED NAMESPACE OR "${NAMESPACE}" STREQUAL "")
    set(NAMESPACE WPEFramework)
endif()

# Target name follows Thunder/Plugin convention: WPEFramework + <PluginName>
set(TARGET ${NAMESPACE}AppGateway)

# Resolve Thunder (WPEFramework) SDK paths relative to this file:
#   ../../dependencies/install
#     include/WPEFramework/...
#     lib/libWPEFrameworkCore.so, libWPEFrameworkPlugins.so, etc.
set(THUNDER_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../../dependencies/install" CACHE PATH "Path to Thunder (WPEFramework) install prefix")

# Normalize to absolute
get_filename_component(THUNDER_INSTALL_PATH_ABS "${THUNDER_INSTALL_PATH}" REALPATH)
set(THUNDER_INCLUDE_DIR "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
set(THUNDER_LIBRARY_DIR "${THUNDER_INSTALL_PATH_ABS}/lib")

if (NOT EXISTS "${THUNDER_INSTALL_PATH_ABS}")
    message(WARNING "Thunder (WPEFramework) SDK not found at ${THUNDER_INSTALL_PATH_ABS}. Adjust THUNDER_INSTALL_PATH if needed.")
endif()

# Requirement: Add include_directories(dependencies/install/include/WPEFramework)
# We satisfy this with the normalized absolute path. Also add as target include.
include_directories("${THUNDER_INCLUDE_DIR}")

# Sources local to this plugin directory
add_library(${TARGET} SHARED
    Module.cpp
    AppGateway.cpp
    Resolver.cpp
    ResolutionStore.cpp
    RequestRouter.cpp
    ConnectionRegistry.cpp
    GatewayWebSocket.cpp
    GatewayClient.cpp
)

# Local and SDK includes
target_include_directories(${TARGET}
    PUBLIC
        "${THUNDER_INCLUDE_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
)

# Use C++11
target_compile_features(${TARGET} PRIVATE cxx_std_11)

# Define Thunder namespace macro for builds that don't inject it from a superproject.
target_compile_definitions(${TARGET} PRIVATE NAMESPACE=${NAMESPACE})

# Link against Thunder libs by resolving full paths explicitly
find_library(THUNDER_CORE_LIB NAMES WPEFrameworkCore HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)
find_library(THUNDER_PLUGINS_LIB NAMES WPEFrameworkPlugins HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)

if (NOT THUNDER_CORE_LIB OR NOT THUNDER_PLUGINS_LIB)
    message(FATAL_ERROR "WPEFramework libraries not found in ${THUNDER_LIBRARY_DIR}. Please ensure dependencies are installed.")
endif()

target_link_libraries(${TARGET}
    PRIVATE
        ${THUNDER_CORE_LIB}
        ${THUNDER_PLUGINS_LIB}
)

set_target_properties(${TARGET}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        OUTPUT_NAME "AppGateway"
)

# Installation paths (conventional Thunder layout)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib/${NAMESPACE_LIB}/plugins COMPONENT libs
)

install(FILES
    AppGateway.conf.in
    AppGateway.config
    DESTINATION etc/${NAMESPACE_LIB}/plugins
)
