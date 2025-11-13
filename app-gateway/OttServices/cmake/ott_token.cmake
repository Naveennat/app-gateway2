# OttServices - ott_token.proto scaffolding (no-op by default)
#
# This file documents how future gRPC/proto stubs will be wired without breaking existing builds.
# It is not included by any top-level CMakeLists.txt in this repo and thus has no effect unless
# explicitly included by a consumer project.
#
# Usage (example):
#   option(OTTSERVICES_ENABLE_OTT_TOKEN "Enable ott_token.proto generated stubs" OFF)
#   if(OTTSERVICES_ENABLE_OTT_TOKEN)
#       find_package(Protobuf REQUIRED)
#       find_package(gRPC REQUIRED)
#       # set(PROTO_DIR "${CMAKE_CURRENT_LIST_DIR}/proto")
#       # set(OTT_TOKEN_PROTO "${PROTO_DIR}/ott_token.proto")
#       #
#       # protobuf_generate_cpp(OTT_TOKEN_PROTO_SRCS OTT_TOKEN_PROTO_HDRS ${OTT_TOKEN_PROTO})
#       # grpc_generate_cpp(OTT_TOKEN_GRPC_SRCS OTT_TOKEN_GRPC_HDRS ${OTT_TOKEN_PROTO})
#       #
#       # add_library(ott_token_proto STATIC
#       #     ${OTT_TOKEN_PROTO_SRCS} ${OTT_TOKEN_PROTO_HDRS}
#       #     ${OTT_TOKEN_GRPC_SRCS} ${OTT_TOKEN_GRPC_HDRS}
#       # )
#       # target_link_libraries(ott_token_proto PUBLIC gRPC::grpc++ protobuf::libprotobuf)
#       #
#       # target_link_libraries(WPEFrameworkOttServices PRIVATE ott_token_proto)
#   endif()
#
# NOTE:
# - Keep this file additive and optional. Do not enable by default.
# - The actual proto and gRPC wiring will be added when the toolchain and proto are available.
