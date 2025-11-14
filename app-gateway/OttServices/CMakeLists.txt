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

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

add_compile_definitions(OTTSERVICES_MAJOR_VERSION=${VERSION_MAJOR})
add_compile_definitions(OTTSERVICES_MINOR_VERSION=${VERSION_MINOR})
add_compile_definitions(OTTSERVICES_PATCH_VERSION=${VERSION_PATCH})

set(MODULE_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(PLUGIN_OTTSERVICES_STARTUPORDER "" CACHE STRING "To configure startup order of OttServices plugin")
set(PLUGIN_OTTSERVICES_AUTOSTART "true" CACHE STRING "Automatically start OttServices plugin")

message("Setup ${MODULE_NAME} v${MODULE_VERSION}")

find_package(${NAMESPACE}Plugins REQUIRED)
find_package(${NAMESPACE}Definitions REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)

# Protobuf and gRPC
# Respect existing project convention: Protobuf via find_package, gRPC via pkg-config variables
find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)

# gRPC C++ plugin for protoc
find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

# ------------------------------------------------------------------------------
# permission_service.proto codegen (legacy behavior: generate into source dir)
# ------------------------------------------------------------------------------
add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --cpp_out ${CMAKE_CURRENT_SOURCE_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
    COMMENT
        "Generating permission_service.pb.cc (protobuf cpp)"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.grpc.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --grpc_out ${CMAKE_CURRENT_SOURCE_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
    COMMENT
        "Generating permission_service.grpc.pb.cc (gRPC cpp)"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
)

add_custom_target(permission_service_protoc ALL
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/permission_service.pb.cc"
        "${CMAKE_CURRENT_SOURCE_DIR}/permission_service.grpc.pb.cc"
)

# ------------------------------------------------------------------------------
# ott_token.proto codegen (NEW): generate into build tree under generated/
# ------------------------------------------------------------------------------
# Detect/find ott_token.proto path within the repo.
# If this file is moved, update OTT_TOKEN_PROTO_SEARCH_PATHS below to include the new relative path(s).
set(OTT_TOKEN_PROTO_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}                    # current dir (app-gateway/OttServices)
    ${CMAKE_CURRENT_SOURCE_DIR}/../protos          # sibling protos dir (if ever moved)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../protos       # higher-level protos dir (if repo reorganizes)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party  # third-party protos (if vendored)
)

find_file(OTT_TOKEN_PROTO
    NAMES ott_token.proto
    PATHS ${OTT_TOKEN_PROTO_SEARCH_PATHS}
    NO_CACHE
)

if (NOT OTT_TOKEN_PROTO)
    message(FATAL_ERROR "Could not locate ott_token.proto. Please update OTT_TOKEN_PROTO_SEARCH_PATHS in ${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt if the file moved.")
endif()

get_filename_component(OTT_TOKEN_PROTO_DIR "${OTT_TOKEN_PROTO}" DIRECTORY)
set(OTT_TOKEN_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Ensure the generated directory exists
file(MAKE_DIRECTORY "${OTT_TOKEN_GEN_DIR}")

# Generate C++ protobuf sources for ott_token.proto into ${OTT_TOKEN_GEN_DIR}
set(OTT_TOKEN_PB_CC  "${OTT_TOKEN_GEN_DIR}/ott_token.pb.cc")
set(OTT_TOKEN_PB_H   "${OTT_TOKEN_GEN_DIR}/ott_token.pb.h")
set(OTT_TOKEN_GRPC_CC "${OTT_TOKEN_GEN_DIR}/ott_token.grpc.pb.cc")
set(OTT_TOKEN_GRPC_H  "${OTT_TOKEN_GEN_DIR}/ott_token.grpc.pb.h")

add_custom_command(
    OUTPUT
        "${OTT_TOKEN_PB_CC}" "${OTT_TOKEN_PB_H}"
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE}
        --cpp_out "${OTT_TOKEN_GEN_DIR}"
        -I "${OTT_TOKEN_PROTO_DIR}"
        "${OTT_TOKEN_PROTO}"
    DEPENDS
        "${OTT_TOKEN_PROTO}"
    COMMENT
        "Generating ott_token.pb.cc/.h (protobuf cpp) into ${OTT_TOKEN_GEN_DIR}"
)

add_custom_command(
    OUTPUT
        "${OTT_TOKEN_GRPC_CC}" "${OTT_TOKEN_GRPC_H}"
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE}
        --grpc_out "${OTT_TOKEN_GEN_DIR}"
        --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
        -I "${OTT_TOKEN_PROTO_DIR}"
        "${OTT_TOKEN_PROTO}"
    DEPENDS
        "${OTT_TOKEN_PROTO}"
    COMMENT
        "Generating ott_token.grpc.pb.cc/.h (gRPC cpp) into ${OTT_TOKEN_GEN_DIR}"
)

# Group the ott_token generated outputs into a dedicated custom target
add_custom_target(ott_token_protos
    DEPENDS
        "${OTT_TOKEN_PB_CC}" "${OTT_TOKEN_PB_H}"
        "${OTT_TOKEN_GRPC_CC}" "${OTT_TOKEN_GRPC_H}"
)

# Create a STATIC library from the generated sources to link against consumers
add_library(ott_token_proto STATIC
    "${OTT_TOKEN_PB_CC}"
    "${OTT_TOKEN_GRPC_CC}"
)
add_dependencies(ott_token_proto ott_token_protos)

# PUBLIC include directories allow consumers (e.g., OttServices) to find generated headers
target_include_directories(ott_token_proto
    PUBLIC
        "${OTT_TOKEN_GEN_DIR}"
        "${OTT_TOKEN_PROTO_DIR}"
)

# Note:
# We do not explicitly link protobuf/grpc here to avoid duplicating link interfaces,
# as the main ${MODULE_NAME} target already links them. If your toolchain requires
# it, uncomment the following lines:
# target_link_libraries(ott_token_proto PUBLIC ${Protobuf_LIBRARIES} ${GRPC_LIBRARIES})

# ------------------------------------------------------------------------------
# OttServices library target
# ------------------------------------------------------------------------------
add_library(${MODULE_NAME} SHARED
        # Note: Keep existing sources to avoid breaking behavior
        OttPermissionsImplementation.cpp
        OttPermissionCache.cpp
        PermissionsClient.cpp
        # Add TokenClient to ensure ott_token usage compiles in this component
        TokenClient.cpp
        OttServices.cpp
        permission_service.pb.cc
        permission_service.grpc.pb.cc
        Module.cpp)

# Ensure OttServices depends on generated protobufs before compiling sources that include them
add_dependencies(${MODULE_NAME}
    permission_service_protoc
    ott_token_protos
)

# Include directories for OttServices
# - ../helpers (existing project convention)
# - ${GRPC_INCLUDE_DIRS} from pkg-config
# - ${OTT_TOKEN_GEN_DIR} to find generated ott_token headers
# - ${OTT_TOKEN_PROTO_DIR} to include the proto source dir (for any relative includes)
target_include_directories(${MODULE_NAME}
    PRIVATE
        ../helpers
        ${GRPC_INCLUDE_DIRS}
        "${OTT_TOKEN_GEN_DIR}"
        "${OTT_TOKEN_PROTO_DIR}"
)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_compile_definitions(${MODULE_NAME} PRIVATE MODULE_NAME=Plugin_${PLUGIN_NAME})
target_compile_options(${MODULE_NAME} PRIVATE ${GRPC_CFLAGS_OTHER})

# Link with required libs and the newly created ott_token_proto library
target_link_libraries(${MODULE_NAME}
        PRIVATE
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions
        ott_token_proto
        ${Protobuf_LIBRARIES}
        ${GRPC_LIBRARIES})

# To error any missing links
target_link_options(${MODULE_NAME} PRIVATE -Wl,-z,defs)
# To error any warnings
target_compile_options(${MODULE_NAME} PRIVATE -Wall -Werror)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})

# Helpful notes:
# - If ott_token.proto moves, update OTT_TOKEN_PROTO_SEARCH_PATHS above.
# - Generated files are placed under ${CMAKE_CURRENT_BINARY_DIR}/generated to keep the source tree clean.
# - If your toolchain provides imported targets (e.g., protobuf::libprotobuf, gRPC::grpc++),
#   you can switch to them by replacing ${Protobuf_LIBRARIES}/${GRPC_LIBRARIES} accordingly.
