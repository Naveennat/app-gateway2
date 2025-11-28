cmake_minimum_required(VERSION 3.10)

project(App2AppProviderTesting CXX)

# Explicit include and lib paths (no find_package)
set(WPEFRAMEWORK_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../../dependencies/install/include/WPEFramework")
set(WPEFRAMEWORK_LIB_DIR "${CMAKE_SOURCE_DIR}/../../dependencies/install/lib")

include_directories(
    ${WPEFRAMEWORK_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/App2AppProvider
)

link_directories(
    ${WPEFRAMEWORK_LIB_DIR}
)

set(SRC_APP2APP
    App2AppProvider/Module.cpp
    App2AppProvider/App2AppProvider.cpp
    App2AppProvider/App2AppProviderImplementation.cpp
)

add_library(App2AppProviderTest SHARED
    ${SRC_APP2APP}
)

# Define module name symbol used by Thunder module macros
target_compile_definitions(App2AppProviderTest PRIVATE MODULE_NAME=App2AppProvider)

# Set C++ standard
target_compile_features(App2AppProviderTest PRIVATE cxx_std_17)

# Thunder/WPEFramework usually requires linking with these libraries; adjust as available in dependencies/install/lib
# Common libs: WPEFrameworkCore, WPEFrameworkPlugins (for IPlugin/IShell), pthread, dl, rt (platform dependent)
target_link_libraries(App2AppProviderTest
    WPEFrameworkCore
    WPEFrameworkPlugins
)

set_target_properties(App2AppProviderTest PROPERTIES
    OUTPUT_NAME "App2AppProviderTest"
    PREFIX ""  # Thunder plugins often use no 'lib' prefix
    SUFFIX ".so"
)
