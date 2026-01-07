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
        # Compute app-gateway2 root from this folder:
        #   tests/l0/appgateway/l0test -> ../../../.. == app-gateway2
        get_filename_component(REPO_ROOT "${CMAKE_SOURCE_DIR}/../../../.." ABSOLUTE)
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

# Ensure CMake can locate WPEFramework*Config.cmake packages from the vendored SDK.
# This is required for reliable discovery of libraries and include directories.
list(PREPEND CMAKE_PREFIX_PATH "${PREFIX}")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "CMake search prefix path" FORCE)
message(STATUS "CMAKE_PREFIX_PATH='${CMAKE_PREFIX_PATH}'")

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
# Repo layout: tests/l0/appgateway/l0test -> ../../../../interfaces == app-gateway2/interfaces
set(APPGW_LOCAL_IFACE_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/../../../../interfaces/IAppGateway.h")
set(APPGW_INSTALLED_IFACE_HEADER "${PREFIX}/include/WPEFramework/interfaces/IAppGateway.h")
if(EXISTS "${APPGW_LOCAL_IFACE_HEADER}")
    message(STATUS "AppGateway l0test: Using local interfaces stub: ${APPGW_LOCAL_IFACE_HEADER}")
elseif(EXISTS "${APPGW_INSTALLED_IFACE_HEADER}")
    message(STATUS "AppGateway l0test: Using installed WPEFramework interface: ${APPGW_INSTALLED_IFACE_HEADER}")
else()
    message(WARNING "AppGateway l0test: interfaces/IAppGateway.h not found locally at ${APPGW_LOCAL_IFACE_HEADER} nor installed at ${APPGW_INSTALLED_IFACE_HEADER}. Building may fail; ensure a stub exists under app-gateway2/interfaces or adjust PREFIX/CMAKE_PREFIX_PATH.")
endif()

# IMPORTANT:
# Prefer local shim headers (plugins/*, core/*, com/*, interfaces/*) over SDK ones.
# This prevents the build from accidentally including SDK headers that drag in heavy
# websocket/messaging subsystems not needed for L0.
target_include_directories(appgateway_l0test BEFORE PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}                                               # this folder (ServiceMock.h, ContextUtils.h, etc.)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../                                   # repo root (so <interfaces/...>, <plugins/...>, <core/...>, <com/...> resolve)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../plugin/AppGateway                  # Plugin headers (read-only, no sources copied into tests)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../helpers                           # helpers under app-gateway2 (as needed by test harness)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../helpers/rdkservices-comcast/helpers # provides UtilsLogging.h + StringUtils.h (required by plugin headers)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../interfaces                        # repo interface stubs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../../interfaces/json
)

# SDK include paths come AFTER local shims.
target_include_directories(appgateway_l0test PRIVATE
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

# Prefer exported CMake package targets from the vendored SDK:
#   ${PREFIX}/lib/cmake/WPEFramework*/WPEFramework*Config.cmake
# This ensures correct transitive link dependencies and include directories.
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(_have_wpe_targets FALSE)
find_package(WPEFrameworkCore QUIET)
find_package(WPEFrameworkMessaging QUIET)
find_package(WPEFrameworkCOM QUIET)
find_package(WPEFrameworkWebSocket QUIET)
find_package(WPEFrameworkCryptalgo QUIET)
find_package(WPEFrameworkPlugins QUIET)

if(TARGET WPEFrameworkCore::WPEFrameworkCore)
    set(_have_wpe_targets TRUE)
    target_link_libraries(appgateway_l0test PRIVATE WPEFrameworkCore::WPEFrameworkCore)
endif()
if(TARGET WPEFrameworkMessaging::WPEFrameworkMessaging)
    target_link_libraries(appgateway_l0test PRIVATE WPEFrameworkMessaging::WPEFrameworkMessaging)
endif()
if(TARGET WPEFrameworkCOM::WPEFrameworkCOM)
    target_link_libraries(appgateway_l0test PRIVATE WPEFrameworkCOM::WPEFrameworkCOM)
endif()
if(TARGET WPEFrameworkWebSocket::WPEFrameworkWebSocket)
    target_link_libraries(appgateway_l0test PRIVATE WPEFrameworkWebSocket::WPEFrameworkWebSocket)
endif()
if(TARGET WPEFrameworkCryptalgo::WPEFrameworkCryptalgo)
    target_link_libraries(appgateway_l0test PRIVATE WPEFrameworkCryptalgo::WPEFrameworkCryptalgo)
endif()
if(TARGET WPEFrameworkPlugins::WPEFrameworkPlugins)
    target_link_libraries(appgateway_l0test PRIVATE WPEFrameworkPlugins::WPEFrameworkPlugins)
endif()

# Fallback to manual discovery if package targets are not available.
if(NOT _have_wpe_targets)
    message(WARNING "WPEFrameworkCore CMake targets not found; falling back to find_library(). Ensure CMAKE_PREFIX_PATH/PREFIX are correct.")

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

    if(WPEFRAMEWORK_CORE_LIB)
        target_link_libraries(appgateway_l0test PRIVATE ${WPEFRAMEWORK_CORE_LIB})
    endif()
    if(WPEFRAMEWORK_MESSAGING_LIB)
        target_link_libraries(appgateway_l0test PRIVATE ${WPEFRAMEWORK_MESSAGING_LIB})
    endif()
    if(WPEFRAMEWORK_PLUGINS_LIB)
        target_link_libraries(appgateway_l0test PRIVATE ${WPEFRAMEWORK_PLUGINS_LIB})
    endif()
endif()

target_link_libraries(appgateway_l0test PRIVATE
    Threads::Threads
    m
    dl
)

# Attempt to link the AppGateway plugin shared object:
#  1) From sibling build tree probe: ${CMAKE_BINARY_DIR}/../AppGateway/libWPEFrameworkAppGateway.so
#     (the runner script creates this symlink if the plugin is installed)
#  2) From installed location (compat):    ${PREFIX}/lib/plugins/libWPEFrameworkAppGateway.so
#  3) From installed location (preferred): ${PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so
set(BUILT_PLUGIN_PATH "${CMAKE_BINARY_DIR}/../AppGateway/libWPEFrameworkAppGateway.so")
set(INSTALLED_PLUGIN_PATH_COMPAT "${PREFIX}/lib/plugins/libWPEFrameworkAppGateway.so")
set(INSTALLED_PLUGIN_PATH_WPE "${PREFIX}/lib/wpeframework/plugins/libWPEFrameworkAppGateway.so")
if(EXISTS "${BUILT_PLUGIN_PATH}")
    message(STATUS "Linking against built AppGateway plugin: ${BUILT_PLUGIN_PATH}")
    target_link_libraries(appgateway_l0test PRIVATE "${BUILT_PLUGIN_PATH}")
elseif(EXISTS "${INSTALLED_PLUGIN_PATH_WPE}")
    message(STATUS "Linking against installed AppGateway plugin: ${INSTALLED_PLUGIN_PATH_WPE}")
    target_link_libraries(appgateway_l0test PRIVATE "${INSTALLED_PLUGIN_PATH_WPE}")
elseif(EXISTS "${INSTALLED_PLUGIN_PATH_COMPAT}")
    message(STATUS "Linking against installed AppGateway plugin: ${INSTALLED_PLUGIN_PATH_COMPAT}")
    target_link_libraries(appgateway_l0test PRIVATE "${INSTALLED_PLUGIN_PATH_COMPAT}")
else()
    message(STATUS "AppGateway plugin not found at: ${BUILT_PLUGIN_PATH}, ${INSTALLED_PLUGIN_PATH_WPE}, or ${INSTALLED_PLUGIN_PATH_COMPAT}. Tests may still build but could fail at runtime.")
endif()

# Set RPATH so test executable runs without needing LD_LIBRARY_PATH
# BUILD_RPATH: allow loading from the executable's directory ($ORIGIN) and ${PREFIX}/lib
# INSTALL_RPATH: ${PREFIX}/lib
set_target_properties(appgateway_l0test PROPERTIES
    # $ORIGIN keeps local build-tree probing working; PREFIX paths cover vendored SDK runtime.
    BUILD_RPATH "$ORIGIN;${PREFIX}/lib;${PREFIX}/lib/wpeframework"
    INSTALL_RPATH "${PREFIX}/lib;${PREFIX}/lib/wpeframework"
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
