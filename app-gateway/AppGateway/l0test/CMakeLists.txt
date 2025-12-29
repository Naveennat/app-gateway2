cmake_minimum_required(VERSION 3.16)

# Standalone build for AppGateway L0 tests from within the l0test directory
project(appgateway_l0test LANGUAGES CXX)

# C++ standard requirement
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Option: Enable warnings for this target
option(APPGW_TEST_ENABLE_WARNINGS "Enable additional compiler warnings for appgateway_l0test" ON)

# Provide a configurable PREFIX pointing to WPEFramework/Thunder install.
# Priority order:
#  1) -DPREFIX=...
#  2) $ENV{PREFIX}
#  3) repo-root/dependencies/install (derived from this CMakeLists.txt location)
#  4) $ENV{PWD}/dependencies/install (fallback)
if(NOT DEFINED PREFIX OR "${PREFIX}" STREQUAL "")
    if(DEFINED ENV{PREFIX} AND NOT "$ENV{PREFIX}" STREQUAL "")
        set(PREFIX "$ENV{PREFIX}")
    else()
        # Compute repository root from this folder (../.. -> app-gateway, ../../.. -> app-gateway2, ../../../.. -> repo root)
        get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}/../../.." ABSOLUTE)
        if(EXISTS "${REPO_ROOT}/dependencies/install")
            set(PREFIX "${REPO_ROOT}/dependencies/install")
        elseif(DEFINED ENV{PWD} AND EXISTS "$ENV{PWD}/dependencies/install")
            set(PREFIX "$ENV{PWD}/dependencies/install")
        else()
            # Default even if not found on disk — user can override during configure
            set(PREFIX "${REPO_ROOT}/dependencies/install")
        endif()
    endif()
endif()
set(PREFIX "${PREFIX}" CACHE STRING "Install prefix to locate Thunder/WPEFramework artifacts (include/lib/plugins)" FORCE)

message(STATUS "Using PREFIX='${PREFIX}'")

# Sources for the L0 test executable (relative paths from this l0test directory)
set(APPGW_L0_SOURCES
    AppGatewayTest.cpp
    AppGateway_Init_DeinitTests.cpp
    AppGateway_JsonRpcResolveTests.cpp
    Resolver_Configure_And_ResolveTests.cpp
    Responder_BehaviorTests.cpp
    ContextUtils_ConversionTests.cpp
)

add_executable(appgateway_l0test ${APPGW_L0_SOURCES})

# Define MODULE_NAME for Thunder/WPEFramework Module.h users
target_compile_definitions(appgateway_l0test PRIVATE MODULE_NAME=AppGateway_L0Test)

# Coverage flags for this target only
# -O0, -g for debug; --coverage for GCC/Clang instrumented coverage
target_compile_options(appgateway_l0test PRIVATE -O0 -g --coverage)
target_link_options(appgateway_l0test PRIVATE --coverage)

# Warnings as requested (optional)
if(APPGW_TEST_ENABLE_WARNINGS)
    target_compile_options(appgateway_l0test PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# Include directories required by tests and plugin code
# Notes:
#  - ..                         -> AppGateway
#  - ../..                      -> app-gateway (so <interfaces/...> resolves)
#  - ../../helpers              -> project helpers under app-gateway
#  - ../../../Supporting_Files  -> shared supporting files at the repository container root
#  - ../../interfaces/*         -> local interface stubs
#  - ${PREFIX}/include (+/WPEFramework) -> installed WPEFramework and interfaces headers
#  - CMAKE_PREFIX_PATH roots (include and include/WPEFramework) if provided
#
# Verify presence of interfaces/IAppGateway.h and warn if not found locally/installed.
set(APPGW_LOCAL_IFACE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/IAppGateway.h")
set(APPGW_INSTALLED_IFACE_HEADER "${PREFIX}/include/WPEFramework/interfaces/IAppGateway.h")
if(EXISTS "${APPGW_LOCAL_IFACE_HEADER}")
    message(STATUS "AppGateway l0test: Using local interfaces stub: ${APPGW_LOCAL_IFACE_HEADER}")
elseif(EXISTS "${APPGW_INSTALLED_IFACE_HEADER}")
    message(STATUS "AppGateway l0test: Using installed WPEFramework interface: ${APPGW_INSTALLED_IFACE_HEADER}")
else()
    message(WARNING "AppGateway l0test: interfaces/IAppGateway.h not found locally at ${APPGW_LOCAL_IFACE_HEADER} nor installed at ${APPGW_INSTALLED_IFACE_HEADER}. Building may fail; ensure a stub exists under app-gateway/interfaces or adjust PREFIX/CMAKE_PREFIX_PATH.")
endif()

target_include_directories(appgateway_l0test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                      # this folder (ServiceMock.h, etc.)
    ${CMAKE_CURRENT_SOURCE_DIR}/..                  # AppGateway
    ${CMAKE_CURRENT_SOURCE_DIR}/../..               # app-gateway (so <interfaces/...> resolves)
    ${CMAKE_SOURCE_DIR}/../..                       # app-gateway (from CMAKE_SOURCE_DIR for standalone builds)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../helpers       # helpers under app-gateway
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Supporting_Files # repo-level Supporting_Files
    ${CMAKE_CURRENT_LIST_DIR}/../../Supporting_Files # app-gateway/Supporting_Files if present
    ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces    # local interface headers (direct includes)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../interfaces/json
    ${PREFIX}/include
    ${PREFIX}/include/WPEFramework
)

# Add include roots from CMAKE_PREFIX_PATH if provided (Thunder/WPEFramework/ThunderInterfaces)
if(CMAKE_PREFIX_PATH)
    foreach(_prefix IN LISTS CMAKE_PREFIX_PATH)
        if(EXISTS "${_prefix}/include")
            target_include_directories(appgateway_l0test PRIVATE "${_prefix}/include")
        endif()
        if(EXISTS "${_prefix}/include/WPEFramework")
            target_include_directories(appgateway_l0test PRIVATE "${_prefix}/include/WPEFramework")
        endif()
    endforeach()
endif()

# Link directories for Thunder/WPEFramework libs
# Default to ${PREFIX}/lib
if(EXISTS "${PREFIX}/lib")
    target_link_directories(appgateway_l0test PRIVATE "${PREFIX}/lib")
endif()

# Find Thunder/WPEFramework libraries with fallback names
# WPEFramework has transitioned from "Thunder" to "WPEFramework" naming,
# so we search for both to be robust.
find_library(WPEFRAMEWORK_CORE_LIB
    NAMES WPEFrameworkCore ThunderCore
    HINTS "${PREFIX}/lib"
)
find_library(WPEFRAMEWORK_MESSAGING_LIB
    NAMES WPEFrameworkMessaging ThunderMessaging
    HINTS "${PREFIX}/lib"
)
find_library(WPEFRAMEWORK_PLUGINS_LIB
    NAMES WPEFrameworkPlugins ThunderPlugins
    HINTS "${PREFIX}/lib"
)

# Optional: libc, dl, pthread
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Link the core Thunder/WPEFramework libs (if found) and standard libs
# We do not mark them as REQUIRED to keep this file usable across environments;
# missing libraries will cause link errors which is clearer than a configure fail.
if(WPEFRAMEWORK_CORE_LIB)
    target_link_libraries(appgateway_l0test PRIVATE ${WPEFRAMEWORK_CORE_LIB})
else()
    message(STATUS "WPEFrameworkCore/ThunderCore not found via find_library; ensure PREFIX is correct.")
endif()

if(WPEFRAMEWORK_MESSAGING_LIB)
    target_link_libraries(appgateway_l0test PRIVATE ${WPEFRAMEWORK_MESSAGING_LIB})
else()
    message(STATUS "WPEFrameworkMessaging/ThunderMessaging not found via find_library; ensure PREFIX is correct.")
endif()

if(WPEFRAMEWORK_PLUGINS_LIB)
    target_link_libraries(appgateway_l0test PRIVATE ${WPEFRAMEWORK_PLUGINS_LIB})
else()
    message(STATUS "WPEFrameworkPlugins/ThunderPlugins not found via find_library; ensure PREFIX is correct.")
endif()

target_link_libraries(appgateway_l0test PRIVATE
    Threads::Threads
    m
    dl
)

# Attempt to link the AppGateway plugin shared object:
#  1) From sibling build tree: ${CMAKE_BINARY_DIR}/../AppGateway/libWPEFrameworkAppGateway.so
#  2) From installed location: ${PREFIX}/lib/plugins/libWPEFrameworkAppGateway.so
set(BUILT_PLUGIN_PATH "${CMAKE_BINARY_DIR}/../AppGateway/libWPEFrameworkAppGateway.so")
set(INSTALLED_PLUGIN_PATH "${PREFIX}/lib/plugins/libWPEFrameworkAppGateway.so")
if(EXISTS "${BUILT_PLUGIN_PATH}")
    message(STATUS "Linking against built AppGateway plugin: ${BUILT_PLUGIN_PATH}")
    target_link_libraries(appgateway_l0test PRIVATE "${BUILT_PLUGIN_PATH}")
elseif(EXISTS "${INSTALLED_PLUGIN_PATH}")
    message(STATUS "Linking against installed AppGateway plugin: ${INSTALLED_PLUGIN_PATH}")
    target_link_libraries(appgateway_l0test PRIVATE "${INSTALLED_PLUGIN_PATH}")
else()
    message(STATUS "AppGateway plugin not found at: ${BUILT_PLUGIN_PATH} or ${INSTALLED_PLUGIN_PATH}. Tests may still build but could fail at runtime.")
endif()

# Set RPATH so test executable runs without needing LD_LIBRARY_PATH
# BUILD_RPATH: allow loading from the executable's directory ($ORIGIN) and ${PREFIX}/lib
# INSTALL_RPATH: ${PREFIX}/lib
set_target_properties(appgateway_l0test PROPERTIES
    BUILD_RPATH "$ORIGIN;${PREFIX}/lib"
    INSTALL_RPATH "${PREFIX}/lib"
)

# 'run' target to execute the test binary.
# The environment is inherited from the shell, so APPGATEWAY_RESOLUTIONS_PATH is available if set.
add_custom_target(run
    DEPENDS appgateway_l0test
    COMMAND $<TARGET_FILE:appgateway_l0test>
    COMMENT "Running appgateway_l0test (inherits APPGATEWAY_RESOLUTIONS_PATH from shell)"
    VERBATIM
)

# 'coverage' target that runs lcov/genhtml
# Commands:
#   lcov -c -o coverage.info -d ${CMAKE_BINARY_DIR}
#   genhtml -o coverage coverage.info
# Target executes in parent of the build dir (to match prior usage).
find_program(LCOV_EXECUTABLE lcov)
find_program(GENHTML_EXECUTABLE genhtml)

if(NOT LCOV_EXECUTABLE OR NOT GENHTML_EXECUTABLE)
    add_custom_target(coverage
        COMMAND ${CMAKE_COMMAND} -E echo "lcov/genhtml not found. Install lcov to generate coverage report."
        COMMAND ${CMAKE_COMMAND} -E echo "Expected commands: lcov -c -o coverage.info -d ${CMAKE_BINARY_DIR} && genhtml -o coverage coverage.info"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/.."
        COMMENT "Coverage tools not found; skipping coverage data generation."
        VERBATIM
    )
else()
    add_custom_target(coverage
        COMMAND "${LCOV_EXECUTABLE}" -c -o coverage.info -d "${CMAKE_BINARY_DIR}"
        COMMAND "${GENHTML_EXECUTABLE}" -o coverage coverage.info
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/.."
        COMMENT "Generating coverage report with lcov/genhtml"
        VERBATIM
    )
endif()
