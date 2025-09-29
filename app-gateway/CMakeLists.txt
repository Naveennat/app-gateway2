cmake_minimum_required(VERSION 3.16)
project(App2AppProvider CXX)

# Explicit include directories (do not use find_package)
# Root relative paths:
#   - dependencies/install/include/WPEFramework
#   - dependencies/entservices-infra/helpers
#   - plugin source path
get_filename_component(BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(WPEF_INC "${BASE_DIR}/dependencies/install/include/WPEFramework")
set(ENT_INFRA_HELPERS "${BASE_DIR}/dependencies/entservices-infra/helpers")
set(PLUGIN_SRC_DIR "${BASE_DIR}/app-gateway2_testing/App2AppProvider")
set(WPEF_LIB_DIR "${BASE_DIR}/dependencies/install/lib")

include_directories(
    "${WPEF_INC}"
    "${ENT_INFRA_HELPERS}"
    "${PLUGIN_SRC_DIR}"
)

link_directories("${WPEF_LIB_DIR}")

set(SOURCES
    "${PLUGIN_SRC_DIR}/App2AppProvider.cpp"
    "${PLUGIN_SRC_DIR}/App2AppProviderImplementation.cpp"
)

add_library(App2AppProvider SHARED ${SOURCES})

target_compile_features(App2AppProvider PRIVATE cxx_std_17)

# Link against WPEFramework libraries explicitly
target_link_libraries(App2AppProvider
    WPEFrameworkCore
    WPEFrameworkPlugins
    WPEFrameworkCOM
    WPEFrameworkDefinitions
    WPEFrameworkMessaging
    WPEFrameworkWebSocket
)

# Optional: set output name and properties for Thunder plugins, if required
set_target_properties(App2AppProvider PROPERTIES
    OUTPUT_NAME "App2AppProvider"
    PREFIX ""  # Thunder plugins typically do not use 'lib' prefix in deployed names
)

message(STATUS "Configured App2AppProvider build with:")
message(STATUS "  Includes: ${WPEF_INC}, ${ENT_INFRA_HELPERS}, ${PLUGIN_SRC_DIR}")
message(STATUS "  Libs: ${WPEF_LIB_DIR}")
