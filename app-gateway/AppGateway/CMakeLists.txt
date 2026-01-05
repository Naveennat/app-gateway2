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

# Thunder installs may not always provide CMake package config files (e.g. in minimal CI environments).
# Prefer the exported CMake targets when available, but gracefully fall back to header-only includes
# so the plugin can still compile against the SDK headers.
find_package(${NAMESPACE}Plugins QUIET)
find_package(${NAMESPACE}Definitions QUIET)
find_package(CompileSettingsDebug CONFIG REQUIRED)

if (NOT TARGET ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
    message(WARNING "${NAMESPACE}Plugins CMake package not found; falling back to header-only interface target.")
    add_library(${NAMESPACE}Plugins::${NAMESPACE}Plugins INTERFACE IMPORTED)
    # Headers are expected under: <prefix>/include/<NAMESPACE>
    set(_thunder_include_guess "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
    if (EXISTS "${_thunder_include_guess}")
        set_target_properties(${NAMESPACE}Plugins::${NAMESPACE}Plugins PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_thunder_include_guess}"
        )
    endif()
endif()

if (NOT TARGET ${NAMESPACE}Definitions::${NAMESPACE}Definitions)
    message(WARNING "${NAMESPACE}Definitions CMake package not found; falling back to header-only interface target.")
    add_library(${NAMESPACE}Definitions::${NAMESPACE}Definitions INTERFACE IMPORTED)
    set(_thunder_include_guess "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
    if (EXISTS "${_thunder_include_guess}")
        set_target_properties(${NAMESPACE}Definitions::${NAMESPACE}Definitions PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${_thunder_include_guess}"
        )
    endif()
endif()

add_library(${MODULE_NAME} SHARED
        AppGateway.cpp
        AppGatewayImplementation.cpp
        Resolver.cpp
        RequestRouter.cpp
        Module.cpp)

# Include helper headers, resolution configs, top-level interfaces, and shared Supporting_Files
target_include_directories(${MODULE_NAME} PRIVATE ../helpers)
target_include_directories(${MODULE_NAME} PRIVATE ./resolutions)
target_include_directories(${MODULE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}")
target_include_directories(${MODULE_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/../Supporting_Files")

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})

target_link_libraries(${MODULE_NAME} 
        PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)


# To error any missing links
target_link_options(${MODULE_NAME} PRIVATE -Wl,-z,defs)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

# define PLUGIN_PRODUCT_CFG
target_compile_definitions(${MODULE_NAME} PRIVATE
	PLUGIN_PRODUCT_CFG="${PRODUCT_CONFIG_DIR}/appGatewayConfig.json")

# TODO: temporary install gateway.config.json - should be added by Platform
install(FILES appGatewayConfig.json
        DESTINATION "${PRODUCT_CONFIG_DIR}")


install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/resolutions/
        DESTINATION "${PRODUCT_CONFIG_DIR}/app-gateway"
        FILES_MATCHING PATTERN "resolution*.json")

write_config(PLUGINS ${PLUGIN_NAME})
