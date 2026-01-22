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

set(PLUGIN_NAME OttServices)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})
set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

add_compile_definitions(OTTSERVICES_MAJOR_VERSION=${VERSION_MAJOR})
add_compile_definitions(OTTSERVICES_MINOR_VERSION=${VERSION_MINOR})
add_compile_definitions(OTTSERVICES_PATCH_VERSION=${VERSION_PATCH})

set(MODULE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(PLUGIN_OTTSERVICES_MODE "Off" CACHE STRING "Controls if the plugin should run in its own process, in process or remote")
set(PLUGIN_OTTSERVICES_STARTUPORDER "" CACHE STRING "To configure startup order of OttServices plugin")
set(PLUGIN_OTTSERVICES_AUTOSTART "true" CACHE STRING "Automatically start OttServices plugin")

message("Setup ${MODULE_NAME} v${MODULE_VERSION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)


find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)


# Define a target for protobuf file generation
# Removed intermediate target - depending directly on proto file in custom commands

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --cpp_out ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/ -I ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/ ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.proto
    COMMENT
        "Generating proto buffer file"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.proto
)


add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.grpc.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --grpc_out ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/ --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/ ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.proto
    COMMENT
        "Generating grpc file"
     DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.proto
)

add_custom_target(force_regenerate ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.pb.cc"
)

add_custom_target(force_regenerate2 ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/ThorPermissions/permission_service.grpc.pb.cc"
)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --cpp_out ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ -I ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.proto
    COMMENT
        "Generating proto buffer file"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.proto
)


add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.grpc.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --grpc_out ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.proto
    COMMENT
        "Generating grpc file"
     DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.proto
)

add_custom_target(force_regenerate_ott ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.pb.cc"
)

add_custom_target(force_regenerate_ott2 ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ProtoImplementations/OttToken/ott_token.grpc.pb.cc"
)


# Build the plugin entry library (OttServices.so)
add_library(${MODULE_NAME} SHARED
    OttServices.cpp
    Module.cpp
)


add_library(${PLUGIN_IMPLEMENTATION} SHARED
        OttServicesImplementation.cpp
        ProtoImplementations/ThorPermissions/OttPermissionCache.cpp
        ProtoImplementations/ThorPermissions/PermissionsClient.cpp
        ProtoImplementations/OttToken/TokenClient.cpp
        ProtoImplementations/OttToken/TokenCache.cpp
        ProtoImplementations/ThorPermissions/permission_service.pb.cc
        ProtoImplementations/ThorPermissions/permission_service.grpc.pb.cc
        ProtoImplementations/OttToken/ott_token.pb.cc
        ProtoImplementations/OttToken/ott_token.grpc.pb.cc
        Module.cpp)

# Ensure the library depends on the protobuf generation
add_dependencies(${PLUGIN_IMPLEMENTATION} force_regenerate force_regenerate2)
add_dependencies(${PLUGIN_IMPLEMENTATION} force_regenerate_ott force_regenerate_ott2)

target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ProtoImplementations/ThorPermissions)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ProtoImplementations/OttToken)
target_include_directories(${PLUGIN_IMPLEMENTATION} PRIVATE ../helpers ${GRPC_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PRIVATE ../helpers)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

set_target_properties(${PLUGIN_IMPLEMENTATION} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
)

# Use same module name define for logging/tracing consistency
target_compile_definitions(${PLUGIN_IMPLEMENTATION} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})
target_compile_options(${PLUGIN_IMPLEMENTATION} PRIVATE -Wall -Werror ${GRPC_CFLAGS_OTHER})
target_link_options(${PLUGIN_IMPLEMENTATION} PRIVATE -Wl,-z,defs)
target_link_libraries(${PLUGIN_IMPLEMENTATION}
        PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${Protobuf_LIBRARIES}
        ${GRPC_LIBRARIES})

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})
target_compile_options(${MODULE_NAME} PRIVATE ${GRPC_CFLAGS_OTHER})

target_link_libraries(${MODULE_NAME}
        PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ${Protobuf_LIBRARIES}
        ${GRPC_LIBRARIES})

# To error any missing links
target_link_options(${MODULE_NAME} PRIVATE -Wl,-z,defs)
# To error any warnings
target_compile_options(${MODULE_NAME} PRIVATE -Wall -Werror)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

install(TARGETS ${PLUGIN_IMPLEMENTATION}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)


write_config(${PLUGIN_NAME})
