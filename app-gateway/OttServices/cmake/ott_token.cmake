# Unconditional generation and wiring for ott_token.proto within OttServices.
# Provides:
# - Custom commands that generate ott_token.pb.{cc,h} and ott_token.grpc.pb.{cc,h}
# - A custom target `ott_token_codegen` and variables OTT_TOKEN_GENERATED_SRCS/HDRS
#
# Include this file from OttServices/CMakeLists.txt.

# Protobuf and grpc plugin expected to be available; fallback to finding if not provided by parent
if(NOT Protobuf_FOUND)
    find_package(Protobuf REQUIRED)
endif()
if(NOT GRPC_CPP_PLUGIN)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
endif()

# Paths
set(OTT_TOKEN_PROTO "${CMAKE_CURRENT_SOURCE_DIR}/ott_token.proto")
set(OTT_TOKEN_PB_CC   "${CMAKE_CURRENT_SOURCE_DIR}/ott_token.pb.cc")
set(OTT_TOKEN_PB_H    "${CMAKE_CURRENT_SOURCE_DIR}/ott_token.pb.h")
set(OTT_TOKEN_GRPC_CC "${CMAKE_CURRENT_SOURCE_DIR}/ott_token.grpc.pb.cc")
set(OTT_TOKEN_GRPC_H  "${CMAKE_CURRENT_SOURCE_DIR}/ott_token.grpc.pb.h")

# Generate protobuf sources
add_custom_command(
    OUTPUT
        "${OTT_TOKEN_PB_CC}"
        "${OTT_TOKEN_PB_H}"
    COMMAND
        "${Protobuf_PROTOC_EXECUTABLE}" --cpp_out "${CMAKE_CURRENT_SOURCE_DIR}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        "${OTT_TOKEN_PROTO}"
    DEPENDS
        "${OTT_TOKEN_PROTO}"
    COMMENT "Generating C++ sources from ott_token.proto"
    VERBATIM
)

# Generate gRPC sources
add_custom_command(
    OUTPUT
        "${OTT_TOKEN_GRPC_CC}"
        "${OTT_TOKEN_GRPC_H}"
    COMMAND
        "${Protobuf_PROTOC_EXECUTABLE}" --grpc_out "${CMAKE_CURRENT_SOURCE_DIR}"
        --plugin=protoc-gen-grpc="${GRPC_CPP_PLUGIN}"
        -I "${CMAKE_CURRENT_SOURCE_DIR}"
        "${OTT_TOKEN_PROTO}"
    DEPENDS
        "${OTT_TOKEN_PROTO}"
    COMMENT "Generating gRPC C++ sources from ott_token.proto"
    VERBATIM
)

# Aggregate codegen target
add_custom_target(ott_token_codegen ALL
    DEPENDS
        "${OTT_TOKEN_PB_CC}" "${OTT_TOKEN_PB_H}"
        "${OTT_TOKEN_GRPC_CC}" "${OTT_TOKEN_GRPC_H}"
)

# Export variables for parent CMakeLists.txt
set(OTT_TOKEN_GENERATED_SRCS "${OTT_TOKEN_PB_CC};${OTT_TOKEN_GRPC_CC}" PARENT_SCOPE)
set(OTT_TOKEN_GENERATED_HDRS "${OTT_TOKEN_PB_H};${OTT_TOKEN_GRPC_H}"   PARENT_SCOPE)
