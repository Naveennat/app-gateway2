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

find_package(Protobuf REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(GRPC REQUIRED grpc++)

set(PLUGIN_IMPLEMENTATION ${MODULE_NAME}Implementation)

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)


# Define a target for protobuf file generation
# Removed intermediate target - depending directly on proto file in custom commands

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --cpp_out ${CMAKE_CURRENT_SOURCE_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
    COMMENT
        "Generating proto buffer file"
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
)


add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.grpc.pb.cc
    COMMAND
        ${Protobuf_PROTOC_EXECUTABLE} --grpc_out ${CMAKE_CURRENT_SOURCE_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
    COMMENT
        "Generating grpc file"
     DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
)

add_custom_target(force_regenerate ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/permission_service.pb.cc"
)

add_custom_target(force_regenerate2 ALL
    DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/permission_service.grpc.pb.cc"
)

#add_custom_target(protoc ALL
#            COMMAND ${Protobuf_PROTOC_EXECUTABLE} --cpp_out ${CMAKE_CURRENT_SOURCE_DIR} -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
#           DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto"
#           COMMENT "Generating C++ protocol buffer code from permission_service.proto"
#    )

#add_dependencies(${MODULE_NAME} protoc)



#add_custom_target(protoc-gen-grpc ALL
#            COMMAND ${Protobuf_PROTOC_EXECUTABLE} --grpc_out ${CMAKE_CURRENT_SOURCE_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto
#            DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/permission_service.proto"
#            COMMENT "Generating gRPC C++ code from permission_service.proto"
#    )

#add_custom_target(run_all ALL DEPENDS protoc)
#add_dependencies(${MODULE_NAME} protoc-gen-grpc)

#target_link_libraries(${MODULE_NAME} PRIVATE grpc++)
#target_link_libraries(${MODULE_NAME} PRIVATE ${Protobuf_LIBRARIES})




add_library(${MODULE_NAME} SHARED
        OttServicesImplementation.cpp
        OttPermissionCache.cpp
        PermissionsClient.cpp
        OttServices.cpp
        permission_service.pb.cc
        permission_service.grpc.pb.cc
        Module.cpp)

# Ensure the library depends on the protobuf generation
add_dependencies(${MODULE_NAME} force_regenerate force_regenerate2)


target_include_directories(${MODULE_NAME} PRIVATE ../helpers ${GRPC_INCLUDE_DIRS})

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

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
add_compile_options(-Wall -Werror)

install(TARGETS ${MODULE_NAME}
        DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
