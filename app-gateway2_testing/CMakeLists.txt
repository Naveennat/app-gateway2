cmake_minimum_required(VERSION 3.16)

project(App2AppProviderPlugin LANGUAGES CXX)

# C++ standard and PIC for shared library
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Compute project root (container root) to construct manual include/library paths
get_filename_component(PROJECT_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

# Manual include/library paths per request (do not use find_package)
set(WPEFRAMEWORK_INCLUDE_DIR "${PROJECT_ROOT_DIR}/dependencies/install/include/WPEFramework")
set(WPEFRAMEWORK_LIB_DIR     "${PROJECT_ROOT_DIR}/dependencies/install/lib")
set(ENTSERVICES_HELPERS_DIR  "${PROJECT_ROOT_DIR}/dependencies/entservices-infra/helpers")

# Sources
set(APP2APP_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/App2AppProvider")
set(APP2APP_SOURCES
    "${APP2APP_SRC_DIR}/App2AppProvider.cpp"
    "${APP2APP_SRC_DIR}/App2AppProvider.h"
    "${APP2APP_SRC_DIR}/App2AppProviderImplementation.cpp"
    "${APP2APP_SRC_DIR}/App2AppProviderImplementation.h"
    "${APP2APP_SRC_DIR}/Module.cpp"
    "${APP2APP_SRC_DIR}/Module.h"
)

# Target
add_library(App2AppProvider SHARED ${APP2APP_SOURCES})

# Include directories (manual)
target_include_directories(App2AppProvider
    PRIVATE
        "${WPEFRAMEWORK_INCLUDE_DIR}"
        "${ENTSERVICES_HELPERS_DIR}"
        "${APP2APP_SRC_DIR}"
)

# Link directories (manual)
target_link_directories(App2AppProvider
    PRIVATE
        "${WPEFRAMEWORK_LIB_DIR}"
)

# Link against core Thunder libraries directly
# Note: linking against Messaging as tracing/LOG macros can route through messaging infra
target_link_libraries(App2AppProvider
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkPlugins
        WPEFrameworkCOM
        WPEFrameworkMessaging
        pthread
        dl
        rt
)

# Output name is the plugin's shared object name
set_target_properties(App2AppProvider PROPERTIES
    OUTPUT_NAME "App2AppProvider"
)

# Optional: Provide an install hint (kept local to the repo directories if needed later)
# install(TARGETS App2AppProvider DESTINATION lib)
