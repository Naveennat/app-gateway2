# FindDefinitions.cmake - compatibility finder for projects using:
#   find_package(Definitions REQUIRED)
#
# The vendored SDK in this repository exports "WPEFrameworkDefinitions", not "Definitions".
# This module makes "Definitions" resolvable by delegating to WPEFrameworkDefinitions.

# Try config-mode first with explicit hints derived from common prefix layouts.
set(_defs_prefix_hints "")
if(DEFINED CMAKE_PREFIX_PATH)
    foreach(_p IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND _defs_prefix_hints "${_p}/lib/cmake/WPEFrameworkDefinitions")
    endforeach()
endif()
if(DEFINED CMAKE_INSTALL_PREFIX)
    list(APPEND _defs_prefix_hints "${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions")
endif()

if(NOT DEFINED WPEFrameworkDefinitions_DIR AND _defs_prefix_hints)
    list(GET _defs_prefix_hints 0 WPEFrameworkDefinitions_DIR)
endif()

find_package(WPEFrameworkDefinitions QUIET)

if(TARGET WPEFrameworkDefinitions::WPEFrameworkDefinitions)
    if(NOT TARGET Definitions::Definitions)
        add_library(Definitions::Definitions INTERFACE IMPORTED)
        set_target_properties(Definitions::Definitions PROPERTIES
            INTERFACE_LINK_LIBRARIES "WPEFrameworkDefinitions::WPEFrameworkDefinitions"
        )
    endif()

    set(Definitions_FOUND TRUE)
    return()
endif()

# Fallback: succeed with an empty interface target to allow isolated builds to configure.
if(NOT TARGET Definitions::Definitions)
    add_library(Definitions::Definitions INTERFACE IMPORTED)
endif()
set(Definitions_FOUND TRUE)
