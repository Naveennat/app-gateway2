cmake_minimum_required(VERSION 3.10)

project(App2AppProviderPlugin)

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

# Include directories (explicit paths, no find_package)
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../AppGateway
    ../../../dependencies/install/include/WPEFramework
)

# Link directories (explicit)
link_directories(
    ../../../dependencies/install/lib
)

# Link with Thunder libraries directly
target_link_libraries(${TARGET}
    PRIVATE
        WPEFrameworkPlugins
        WPEFrameworkCore
        WPEFrameworkCOM
        WPEFrameworkMessaging
        WPEFrameworkWebSocket
        WPEFrameworkCryptalgo
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
