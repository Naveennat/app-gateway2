cmake_minimum_required(VERSION 3.10)

project(App2AppProviderPlugin)

# Ensure CMake can discover WPEFramework packages from the container-local install
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../dependencies/install")

find_package(WPEFramework REQUIRED CONFIG)
find_package(WPEFrameworkPlugins REQUIRED CONFIG)

set(TARGET App2AppProvider)

# Source files
add_library(${TARGET} SHARED
    Module.cpp
    App2AppProvider.cpp
    ProviderRegistry.cpp
    CorrelationMap.cpp
    GatewayClient.cpp
    PermissionManager.cpp
)

# Local includes for this submodule
target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../AppGateway
)

# Link with imported Thunder (WPEFramework) targets to get proper include/link dirs
target_link_libraries(${TARGET}
    PRIVATE
        WPEFrameworkPlugins::WPEFrameworkPlugins
        WPEFrameworkCore::WPEFrameworkCore
        WPEFrameworkCOM::WPEFrameworkCOM
        WPEFrameworkMessaging::WPEFrameworkMessaging
        WPEFrameworkWebSocket::WPEFrameworkWebSocket
        WPEFrameworkCryptalgo::WPEFrameworkCryptalgo
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME "WPEFrameworkApp2AppProvider"
)

# Install shared library
install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}
    ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}
    RUNTIME DESTINATION bin
)

# Install configuration files
install(FILES
    App2AppProvider.conf.in
    App2AppProvider.config
    DESTINATION share/WPEFramework/App2AppProvider
)
