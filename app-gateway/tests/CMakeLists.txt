cmake_minimum_required(VERSION 3.16)

project(AppGatewayTests CXX)

enable_testing()

# Assume Thunder core/libs already built in this workspace; adjust include paths to local install tree if needed.
include_directories(
    ${CMAKE_SOURCE_DIR}/app-gateway/AppGateway
    ${CMAKE_SOURCE_DIR}/dependencies/Thunder/Source
    ${CMAKE_SOURCE_DIR}/install/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}/install/lib
)

add_executable(test_appgateway
    test_appgateway.cpp
)

# Link to Thunder Core/Plugins as available in this repo's install tree.
# These names are typical; adapt as present in the local build output.
target_link_libraries(test_appgateway
    WPEFrameworkCore
    WPEFrameworkPlugins
)

add_test(NAME AppGateway.Basic COMMAND test_appgateway)
