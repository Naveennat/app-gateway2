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

cmake_minimum_required(VERSION 3.15)

project(AppGateway LANGUAGES C CXX)

# Thunder/WPEFramework convention:
# - NAMESPACE is usually "WPEFramework" (used by package config + imported targets)
# - STORAGE_DIRECTORY is typically "wpeframework"
set(NAMESPACE "WPEFramework" CACHE STRING "Thunder CMake package namespace (e.g. WPEFramework)")
set(STORAGE_DIRECTORY "wpeframework" CACHE STRING "Install storage directory under lib/<storage>/plugins")

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

# Thunder 4.4 SDK exports packages as:
#  - WPEFrameworkPlugins, WPEFrameworkCore, WPEFrameworkDefinitions, ...
find_package(${NAMESPACE}Plugins CONFIG REQUIRED)
find_package(${NAMESPACE}Definitions CONFIG REQUIRED)
find_package(${NAMESPACE}Core CONFIG REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

add_library(${MODULE_NAME} SHARED
        AppGateway.cpp
        AppGatewayImplementation.cpp
        AppGatewayResponderImplementation.cpp
        Resolver.cpp
        Module.cpp)

# -----------------------------------------------------------------------------
# Include paths
#
# IMPORTANT: Prefer entservices-infra helpers BEFORE local shims to avoid
# duplicate/ambiguous definitions (e.g., ErrorUtils, StringUtils, etc.).
# -----------------------------------------------------------------------------

# Prefer vendored entservices-infra helper headers (if present).
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/entservices-infra/helpers)
    target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/entservices-infra/helpers)
endif()

# Include shared Supporting_Files for common utilities like ContextUtils.h and
# callsign constants.
if(EXISTS ${CMAKE_SOURCE_DIR}/../Supporting_Files)
    target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/../Supporting_Files)
endif()

# Local helpers and resolutions (kept after entservices-infra to avoid conflicts)
target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../helpers
        ${CMAKE_CURRENT_SOURCE_DIR}/resolutions)

# Add Thunder include directories explicitly if package config did not propagate
# This helps resolve <interfaces/...> headers like IAppGateway.h
if(CMAKE_PREFIX_PATH)
    target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_PREFIX_PATH}/include
        ${CMAKE_PREFIX_PATH}/include/WPEFramework
        ${CMAKE_PREFIX_PATH}/include/WPEFramework/interfaces
        ${CMAKE_PREFIX_PATH}/include/interfaces)
endif()
# Prefer our local helpers by adding it again at the end of the include search list
target_include_directories(${MODULE_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../helpers)

# If a local interfaces folder exists in this repo, include it as well
if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/../interfaces)
    target_include_directories(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/../interfaces
        ${CMAKE_CURRENT_LIST_DIR}/..) # so <interfaces/...> works
endif()

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

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
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${NAMESPACE}Core::${NAMESPACE}Core
)

# Link Tracing if available in this Thunder installation
if(TARGET ${NAMESPACE}Tracing::${NAMESPACE}Tracing)
    target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Tracing::${NAMESPACE}Tracing)
endif()

# If the Interfaces target exists, link it as well (used by IAppGateway et al.)
if(TARGET ${NAMESPACE}Interfaces::${NAMESPACE}Interfaces)
    target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Interfaces::${NAMESPACE}Interfaces)
endif()

# To error any missing links
target_link_options(${MODULE_NAME} PRIVATE -Wl,-z,defs)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

# Install configs under the install prefix to avoid root-only /etc writes in CI
set(APP_GATEWAY_INSTALL_ETC_DIR "${CMAKE_INSTALL_PREFIX}/etc/app-gateway")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/resolutions/resolution.base.json
        DESTINATION "${APP_GATEWAY_INSTALL_ETC_DIR}")

write_config(PLUGINS ${PLUGIN_NAME})

# -----------------------------------------------------------------------------
# L0 tests moved to: tests/l0/appgateway/l0test
# -----------------------------------------------------------------------------
# The plugin build must not depend on test sources. Unit/L0 tests are built from
# the dedicated test CMake under tests/l0/appgateway/l0test.
#
# (Previously, this repo temporarily embedded an appgateway_l0test target here,
# but after restructuring to multi-plugin layout, those sources no longer exist
# under plugin/AppGateway.)
