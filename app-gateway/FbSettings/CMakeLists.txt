# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

# If not stated otherwise in this file or this component's license file the
# following copyright...
#
# CMake configuration for FbSettings plugin aligned with AppGateway style.

cmake_minimum_required(VERSION 3.10)

# Thunder typically defines the NAMESPACE cache variable as WPEFramework.
if (NOT DEFINED NAMESPACE OR "${NAMESPACE}" STREQUAL "")
    set(NAMESPACE WPEFramework)
endif()

set(PLUGIN_NAME FbSettings)
set(TARGET ${NAMESPACE}${PLUGIN_NAME})

# Version macros for plugin metadata
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
add_compile_definitions(FBSETTINGS_MAJOR_VERSION=${VERSION_MAJOR})
add_compile_definitions(FBSETTINGS_MINOR_VERSION=${VERSION_MINOR})
add_compile_definitions(FBSETTINGS_PATCH_VERSION=${VERSION_PATCH})

# Resolve Thunder (WPEFramework) SDK paths, prefer values propagated from the top-level
# If not provided, fallback to repo-relative path: <repo-root>/app-gateway2/dependencies/install
if (DEFINED THUNDER_INSTALL_PATH_ABS)
    set(_THUNDER_SDK_ROOT "${THUNDER_INSTALL_PATH_ABS}")
elseif (DEFINED THUNDER_INSTALL_PATH)
    get_filename_component(_THUNDER_SDK_ROOT "${THUNDER_INSTALL_PATH}" REALPATH)
else()
    get_filename_component(_THUNDER_SDK_ROOT "${CMAKE_SOURCE_DIR}/../dependencies/install" REALPATH)
endif()
set(THUNDER_INCLUDE_DIR "${_THUNDER_SDK_ROOT}/include/${NAMESPACE}")
set(THUNDER_LIBRARY_DIR "${_THUNDER_SDK_ROOT}/lib")

# Build plugin shared library
add_library(${TARGET} SHARED
    Module.cpp
    FbSettings.cpp
    FbSettingsImplementation.cpp
    delegate/SystemDelegate.cpp
)

# Include directories: Thunder SDK, plugin dir, helpers, Supporting_Files and local interfaces
target_include_directories(${TARGET}
    PUBLIC
        "${THUNDER_INCLUDE_DIR}"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/delegate"
        "${CMAKE_CURRENT_LIST_DIR}/interfaces"
        "${CMAKE_CURRENT_LIST_DIR}/interfaces/json"
        "${CMAKE_CURRENT_LIST_DIR}/../helpers"
        "${CMAKE_SOURCE_DIR}/../Supporting_Files"
        "${CMAKE_SOURCE_DIR}"           # for UtilsLogging.h in parent app-gateway
)

# Use C++11
target_compile_features(${TARGET} PRIVATE cxx_std_11)

# Define MODULE_NAME to enable proper Thunder tracing category setup for this binary
target_compile_definitions(${TARGET} PRIVATE NAMESPACE=${NAMESPACE} MODULE_NAME=Plugin_${PLUGIN_NAME})

# Link against Thunder libs by resolving full paths explicitly
find_library(THUNDER_CORE_LIB NAMES WPEFrameworkCore HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)
find_library(THUNDER_PLUGINS_LIB NAMES WPEFrameworkPlugins HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)
find_library(THUNDER_DEFINITIONS_LIB NAMES WPEFrameworkDefinitions HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)

if (NOT THUNDER_CORE_LIB OR NOT THUNDER_PLUGINS_LIB)
    message(FATAL_ERROR "WPEFramework libraries not found in ${THUNDER_LIBRARY_DIR}. Please ensure dependencies are installed.")
endif()

target_link_libraries(${TARGET}
    PRIVATE
        ${THUNDER_CORE_LIB}
        ${THUNDER_PLUGINS_LIB}
        ${THUNDER_DEFINITIONS_LIB}
)

set_target_properties(${TARGET}
    PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES
        FRAMEWORK FALSE
        OUTPUT_NAME "${PLUGIN_NAME}"
)

# Installation paths (conventional Thunder layout)
string(TOLOWER ${NAMESPACE} NAMESPACE_LIB)

install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib/${NAMESPACE_LIB}/plugins COMPONENT libs
)

install(FILES
    FbSettings.conf.in
    FbSettings.config
    DESTINATION etc/${NAMESPACE_LIB}/plugins
)
