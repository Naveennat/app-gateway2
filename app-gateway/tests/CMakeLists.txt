cmake_minimum_required(VERSION 3.10)

# Do not call project() here to keep PROJECT_SOURCE_DIR pointing to the parent project root.
# enable_testing() is called by the top-level CMakeLists.txt

# Ensure NAMESPACE is available (Thunder typically uses WPEFramework)
if (NOT DEFINED NAMESPACE OR "${NAMESPACE}" STREQUAL "")
    set(NAMESPACE WPEFramework)
endif()

# Reuse Thunder installation hints from parent if available; otherwise compute relative to this directory
if (NOT DEFINED THUNDER_INSTALL_PATH_ABS)
    # When building this test standalone, assume Thunder is installed at ../../dependencies/install
    set(THUNDER_INSTALL_PATH "${CMAKE_CURRENT_LIST_DIR}/../dependencies/install" CACHE PATH "Path to Thunder (WPEFramework) install prefix for tests")
    get_filename_component(THUNDER_INSTALL_PATH_ABS "${THUNDER_INSTALL_PATH}" REALPATH)
endif()
if (NOT DEFINED THUNDER_LIBRARY_DIR)
    set(THUNDER_LIBRARY_DIR "${THUNDER_INSTALL_PATH_ABS}/lib")
endif()
if (NOT DEFINED THUNDER_INCLUDE_DIR)
    set(THUNDER_INCLUDE_DIR "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
endif()

# Find Thunder libs explicitly
find_library(THUNDER_CORE_LIB NAMES WPEFrameworkCore HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)
find_library(THUNDER_PLUGINS_LIB NAMES WPEFrameworkPlugins HINTS "${THUNDER_LIBRARY_DIR}" NO_DEFAULT_PATH)

if (NOT THUNDER_CORE_LIB OR NOT THUNDER_PLUGINS_LIB)
    message(FATAL_ERROR "WPEFramework libraries not found in ${THUNDER_LIBRARY_DIR}. Please ensure dependencies are installed.")
endif()

# Test executable
add_executable(test_appgateway
    test_appgateway.cpp
)

# Includes for the test:
# - Thunder headers
# - AppGateway headers from the parent project source
target_include_directories(test_appgateway
    PUBLIC
        "${THUNDER_INCLUDE_DIR}"
    PRIVATE
        "${PROJECT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/AppGateway"
)

# Build/test with C++11 and ensure NAMESPACE macro is defined (used by Thunder headers)
target_compile_features(test_appgateway PRIVATE cxx_std_11)
target_compile_definitions(test_appgateway PRIVATE NAMESPACE=${NAMESPACE})

# Link to the AppGateway plugin target if present; otherwise rely on Thunder core/plugins
# The plugin target name follows Thunder convention: <NAMESPACE>AppGateway (e.g., WPEFrameworkAppGateway)
set(PLUGIN_TARGET_NAME "${NAMESPACE}AppGateway")
if (TARGET ${PLUGIN_TARGET_NAME})
    target_link_libraries(test_appgateway
        PRIVATE
            ${PLUGIN_TARGET_NAME}
            ${THUNDER_CORE_LIB}
            ${THUNDER_PLUGINS_LIB}
    )
else()
    # Fallback linking (in case tests are built standalone without the plugin target)
    target_link_libraries(test_appgateway
        PRIVATE
            ${THUNDER_CORE_LIB}
            ${THUNDER_PLUGINS_LIB}
    )
endif()

# Register the test with CTest
add_test(NAME AppGateway.Basic COMMAND test_appgateway)
