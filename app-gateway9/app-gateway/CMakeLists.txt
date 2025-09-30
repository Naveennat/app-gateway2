cmake_minimum_required(VERSION 3.10)

project(AppGatewayPlugin)

# Expect Thunder (WPEFramework) to be present as dependency in the workspace.
find_package(WPEFramework REQUIRED)

set(TARGET AppGateway)

add_library(${TARGET} SHARED
    AppGateway/Module.cpp
    AppGateway/AppGateway.cpp
)

# Include directories
target_include_directories(${TARGET}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/AppGateway
)

# Link with Thunder plugins core
target_link_libraries(${TARGET}
    PRIVATE
        WPEFrameworkPlugins::WPEFrameworkPlugins
        WPEFrameworkCore::WPEFrameworkCore
        WPEFrameworkCOM::WPEFrameworkCOM
        WPEFrameworkMessaging::WPEFrameworkMessaging
        WPEFrameworkWebSocket::WPEFrameworkWebSocket
)

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 11
    CXX_STANDARD_REQUIRED YES
    FRAMEWORK FALSE
    VERSION 1.0.0
    SOVERSION 1
    OUTPUT_NAME "WPEFrameworkAppGateway"
)

# Install plugin shared library
install(TARGETS ${TARGET}
    LIBRARY DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}
    ARCHIVE DESTINATION lib/${CMAKE_LIBRARY_ARCHITECTURE}
    RUNTIME DESTINATION bin
)

# Install configuration template
install(FILES
    AppGateway/AppGateway.conf.in
    AppGateway/AppGateway.config
    DESTINATION share/WPEFramework/AppGateway
)

# Also build the App2AppProvider plugin
add_subdirectory(App2AppProvider)

