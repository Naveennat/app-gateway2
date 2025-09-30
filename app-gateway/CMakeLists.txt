cmake_minimum_required(VERSION 3.10)

project(app-gateway2-plugins)

# Root paths for Thunder includes/libs (no find_package as per instruction)
set(THUNDER_INSTALL_DIR ${CMAKE_SOURCE_DIR}/../dependencies/install)
set(THUNDER_INCLUDE_DIR ${THUNDER_INSTALL_DIR}/include)
set(THUNDER_LIB_DIR ${THUNDER_INSTALL_DIR}/lib)

include_directories(
    ${THUNDER_INCLUDE_DIR}
    ${THUNDER_INCLUDE_DIR}/WPEFramework
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/OttServices
    ${CMAKE_SOURCE_DIR}/../Supporting_Files
    ${CMAKE_SOURCE_DIR}/../../Supporting_Files
)

link_directories(
    ${THUNDER_LIB_DIR}
)

# -----------------------------------------------------------------------------
# Protobuf/gRPC integration for permission_service.proto
# Pattern follows entservices-infra-712/CloudStore/CMakeLists.txt
# NOTE: Disabled by default to allow builds in environments without system-wide
# Protobuf/gRPC. Enable with -DENABLE_PERMISSION_PROTO_GRPC=ON to generate and link.
# -----------------------------------------------------------------------------
option(ENABLE_PERMISSION_PROTO_GRPC "Generate and link Protobuf/gRPC stubs for permission_service.proto" OFF)

# OttServices plugin target
set(OTTSERVICES_SOURCES
    ${CMAKE_SOURCE_DIR}/OttServices/Module.cpp
    ${CMAKE_SOURCE_DIR}/OttServices/OttServices.cpp
    ${CMAKE_SOURCE_DIR}/OttServices/OttServicesImplementation.cpp
    ${CMAKE_SOURCE_DIR}/OttServices/PermissionsClient.cpp
)

set(OTTSERVICES_HEADERS
    ${CMAKE_SOURCE_DIR}/OttServices/Module.h
    ${CMAKE_SOURCE_DIR}/OttServices/IOttServices.h
    ${CMAKE_SOURCE_DIR}/OttServices/OttServices.h
    ${CMAKE_SOURCE_DIR}/OttServices/OttServicesImplementation.h
    ${CMAKE_SOURCE_DIR}/OttServices/PermissionsClient.h
)

# Always compile with at least C++11 for this target
add_library(OttServices SHARED
    ${OTTSERVICES_SOURCES}
    ${OTTSERVICES_HEADERS}
)
target_compile_features(OttServices PRIVATE cxx_std_11)

target_compile_definitions(OttServices
    PRIVATE
        MODULE_NAME=OttServices
        PLUGIN_OttServices=1
)

if (ENABLE_PERMISSION_PROTO_GRPC)
    # Find Protobuf and gRPC packages and the grpc C++ plugin for protoc
    find_package(Protobuf REQUIRED)
    find_package(gRPC REQUIRED)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)

    # Paths to proto (now located within the OttServices directory)
    set(PERMISSION_PROTO_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/OttServices)
    set(PERMISSION_PROTO_FILE ${PERMISSION_PROTO_DIR}/permission_service.proto)

    # Generate protobuf C++ sources into the build directory
    add_custom_target(permission_service_protoc
        ${Protobuf_PROTOC_EXECUTABLE}
            --cpp_out ${CMAKE_CURRENT_BINARY_DIR}
            -I ${PERMISSION_PROTO_DIR}
            ${PERMISSION_PROTO_FILE}
        COMMENT "Generating C++ Protobuf sources for permission_service.proto"
        VERBATIM
    )

    # Generate gRPC C++ sources into the build directory
    add_custom_target(permission_service_protoc_grpc
        ${Protobuf_PROTOC_EXECUTABLE}
            --grpc_out ${CMAKE_CURRENT_BINARY_DIR}
            --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN}
            -I ${PERMISSION_PROTO_DIR}
            ${PERMISSION_PROTO_FILE}
        COMMENT "Generating C++ gRPC sources for permission_service.proto"
        VERBATIM
    )

    # Add generated protobuf/grpc sources to target
    target_sources(OttServices PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/permission_service.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/permission_service.grpc.pb.cc
    )
    set_property(SOURCE
        ${CMAKE_CURRENT_BINARY_DIR}/permission_service.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/permission_service.grpc.pb.cc
        PROPERTY GENERATED 1
    )

    # Ensure generation happens before building OttServices
    add_dependencies(OttServices permission_service_protoc permission_service_protoc_grpc)

    # Include the build directory so generated headers are found
    target_include_directories(OttServices
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
    )

    # Enable code path in PermissionsClient for gRPC usage
    target_compile_definitions(OttServices PRIVATE OTTSERVICES_ENABLE_PERMS_GRPC)

    # Link Protobuf and gRPC::grpc++ as requested
    target_link_libraries(OttServices
        PRIVATE
            ${Protobuf_LIBRARIES}
            gRPC::grpc++
    )
endif()

# Link against Thunder core and plugins JSONRPC libraries directly
# The exact library filenames may vary per build; commonly they are WPEFrameworkCore and WPEFrameworkPlugins.
target_link_libraries(OttServices
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkPlugins
)

# Set output name/prefix consistent with Thunder plugins (libOttServices.so)
set_target_properties(OttServices PROPERTIES
    OUTPUT_NAME "OttServices"
    PREFIX "lib"
)

# Install rules (optional in some repos; add if used by superbuild)
install(TARGETS OttServices
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# If other plugins/subdirs exist, you may add_subdirectory() for them here.
