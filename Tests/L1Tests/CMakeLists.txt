cmake_minimum_required(VERSION 3.8)

# This repository's test build is driven through entservices-testframework.
# The authoritative L1Tests implementation lives under:
#   app-gateway2/entservices-testframework/entservices-testframework/Tests/L1Tests
#
# This shim exists to satisfy repository layout requirements while keeping a
# single build source of truth.

add_subdirectory(../../entservices-testframework/entservices-testframework/Tests/L1Tests
                 ${CMAKE_BINARY_DIR}/entservices-testframework-L1Tests)
