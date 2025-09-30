cmake_minimum_required(VERSION 3.10)

project(app-gateway2-plugins)

# Root paths for Thunder includes/libs (no find_package as per instruction)
set(THUNDER_INSTALL_DIR ${CMAKE_SOURCE_DIR}/../dependencies/install)
set(THUNDER_INCLUDE_DIR ${THUNDER_INSTALL_DIR}/include)
set(THUNDER_LIB_DIR ${THUNDER_INSTALL_DIR}/lib)

include_directories(
    ${THUNDER_INCLUDE_DIR}
    ${THUNDER_INCLUDE_DIR}/WPEFramework
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/OttServices
)

link_directories(
    ${THUNDER_LIB_DIR}
)

# OttServices plugin target
set(OTTSERVICES_SOURCES
    ${CMAKE_SOURCE_DIR}/OttServices/OttServices.cpp
    ${CMAKE_SOURCE_DIR}/OttServices/OttServicesImplementation.cpp
)

set(OTTSERVICES_HEADERS
    ${CMAKE_SOURCE_DIR}/OttServices/OttServices.h
    ${CMAKE_SOURCE_DIR}/OttServices/OttServicesImplementation.h
)

add_library(OttServices SHARED
    ${OTTSERVICES_SOURCES}
    ${OTTSERVICES_HEADERS}
)

target_compile_definitions(OttServices
    PRIVATE
        MODULE_NAME=OttServices
        PLUGIN_OttServices=1
)

# Link against Thunder core and plugins JSONRPC libraries directly
# The exact library filenames may vary per build; commonly they are ThunderCore, ThunderPlugins, and ThunderInterfaces.
# We keep it conservative and add common ones that exist in dependencies/install/lib.
target_link_libraries(OttServices
    PRIVATE
        WPEFrameworkCore
        WPEFrameworkPlugins
)

# Set output name/prefix consistent with Thunder plugins (libOttServices.so)
set_target_properties(OttServices PROPERTIES
    OUTPUT_NAME "OttServices"
    PREFIX "lib"
)

# Install rules (optional in some repos; add if used by superbuild)
install(TARGETS OttServices
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# If other plugins/subdirs exist, you may add_subdirectory() for them here.
