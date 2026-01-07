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

set(PLUGIN_NAME AppGateway)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

add_compile_definitions(APPGATEWAY_MAJOR_VERSION=${VERSION_MAJOR})
add_compile_definitions(APPGATEWAY_MINOR_VERSION=${VERSION_MINOR})
add_compile_definitions(APPGATEWAY_PATCH_VERSION=${VERSION_PATCH})

set(MODULE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(PLUGIN_APPGATEWAY_STARTUPORDER "" CACHE STRING "To configure startup order of AppGateway plugin")
set(PLUGIN_APPGATEWAY_AUTOSTART "false" CACHE STRING "Automatically start AppGateway plugin")

message("Setup ${MODULE_NAME} v${MODULE_VERSION}")

# Ensure standard install directories are defined (SYSCONFDIR, LIBDIR, etc.).
# Without this, CMAKE_INSTALL_SYSCONFDIR can be empty and the config install
# destination becomes an absolute path like "/app-gateway" (which fails in CI).
include(GNUInstallDirs)
if(NOT DEFINED CMAKE_INSTALL_SYSCONFDIR OR "${CMAKE_INSTALL_SYSCONFDIR}" STREQUAL "")
    set(CMAKE_INSTALL_SYSCONFDIR "etc")
endif()

# Enforce upstream-expected C++ standard at the project level.
# (User request: try C++11 first; fallback handled by build script.)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
        AppGateway.cpp
        AppGatewayImplementation.cpp
        AppGatewayResponderImplementation.cpp
        Resolver.cpp
        Module.cpp)

# Include plugin-local headers first (e.g., Module.h is located next to sources)
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Plugin-local resolutions
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/resolutions)

# Prefer repo-local stub JSON headers (e.g., interfaces/json/JAppGatewayResolver.h)
# over SDK placeholders under dependencies/install/include/WPEFramework/interfaces/json.
# This also makes repo-local interfaces (app-gateway2/interfaces/...) visible.
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../..)

# Make repo interfaces directly reachable via the normal include mechanism.
# This is required so that Module.h's include chain can resolve:
#   #include <Ids.h>
# without modifying any plugin/AppGateway .cpp/.h files or force-including headers.
#
# Use an absolute path derived from PROJECT_SOURCE_DIR (PLUGIN_SRC root) to avoid
# dependence on the caller's current working directory.
get_filename_component(APPGATEWAY_REPO_INTERFACES_DIR
    "${PROJECT_SOURCE_DIR}/../../interfaces"
    ABSOLUTE
)
target_include_directories(${MODULE_NAME} PRIVATE ${APPGATEWAY_REPO_INTERFACES_DIR})

# Allow the build script to inject additional include directories while keeping
# deterministic precedence (plugin-local + repo-local first).
#
# IMPORTANT:
# - This is expected to be provided as a CMake cache variable (LIST):
#     -DAPPGATEWAY_EXTRA_INCLUDE_DIRS:LIST="dir1;dir2;dir3"
# - We append it AFTER existing include dirs so plugin-local and repo-local stubs
#   remain highest precedence, while helper injection becomes available.
# - The build script keeps helpers/rdkservices-comcast/helpers first, then
#   helpers/entservices-infra/helpers, plus repo-root helpers and interfaces.
set(APPGATEWAY_EXTRA_INCLUDE_DIRS "" CACHE STRING "Extra include directories for AppGateway target (CMake list)")
if(NOT "${APPGATEWAY_EXTRA_INCLUDE_DIRS}" STREQUAL "")
    target_include_directories(${MODULE_NAME} PRIVATE ${APPGATEWAY_EXTRA_INCLUDE_DIRS})
endif()

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})

# Add platform-specific paths
if(BUILD_CONFIG_PATH)
    target_compile_definitions(${MODULE_NAME} PRIVATE BUILD_CONFIG_PATH="${BUILD_CONFIG_PATH}")
endif()

if(VENDOR_CONFIG_PATH)
    target_compile_definitions(${MODULE_NAME} PRIVATE VENDOR_CONFIG_PATH="${VENDOR_CONFIG_PATH}")
endif()

# Add App Gateway Automation definitions if enabled
if(ENABLE_APP_GATEWAY_AUTOMATION)
    target_compile_definitions(${MODULE_NAME} PRIVATE ENABLE_APP_GATEWAY_AUTOMATION)
    if(AUTOMATION_APP_ID)
        target_compile_definitions(${MODULE_NAME} PRIVATE AUTOMATION_APP_ID=${AUTOMATION_APP_ID})
    endif()
    if(APP_GATEWAY_ENHANCED_LOGGING_INDICATOR)
        target_compile_definitions(${MODULE_NAME} PRIVATE APP_GATEWAY_ENHANCED_LOGGING_INDICATOR=${APP_GATEWAY_ENHANCED_LOGGING_INDICATOR})
    endif()
endif()

target_link_libraries(${MODULE_NAME}
        PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

# To error any missing links
target_link_options(${MODULE_NAME} PRIVATE -Wl,-z,defs)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

# Install under the configured sysconf dir (default becomes <prefix>/etc),
# avoiding writes to system /etc during local builds.
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resolutions/resolution.base.json
        DESTINATION "${CMAKE_INSTALL_SYSCONFDIR}/app-gateway")

write_config(${PLUGIN_NAME})
