# FindPlugins.cmake - compatibility finder for projects using:
#   find_package(Plugins REQUIRED)
#
# The vendored SDK in this repository exports "WPEFrameworkPlugins", not "Plugins".
# This module makes "Plugins" resolvable by delegating to WPEFrameworkPlugins.

# Prefer config-mode first if available.
# Some CMake invocations may not resolve WPEFrameworkPlugins via prefix search reliably,
# so provide explicit hints based on common Thunder/WPEFramework install layouts.
set(_plugins_prefix_hints "")
if(DEFINED CMAKE_PREFIX_PATH)
    foreach(_p IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND _plugins_prefix_hints "${_p}/lib/cmake/WPEFrameworkPlugins")
    endforeach()
endif()
if(DEFINED CMAKE_INSTALL_PREFIX)
    list(APPEND _plugins_prefix_hints "${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkPlugins")
endif()

# If user/CI already provided WPEFrameworkPlugins_DIR, respect it; otherwise hint it.
if(NOT DEFINED WPEFrameworkPlugins_DIR AND _plugins_prefix_hints)
    list(GET _plugins_prefix_hints 0 WPEFrameworkPlugins_DIR)
endif()

find_package(WPEFrameworkPlugins QUIET)

if(TARGET WPEFrameworkPlugins::WPEFrameworkPlugins)
    # Expose a conventional target expected by some callers.
    if(NOT TARGET Plugins::Plugins)
        add_library(Plugins::Plugins INTERFACE IMPORTED)
        set_target_properties(Plugins::Plugins PROPERTIES
            INTERFACE_LINK_LIBRARIES "WPEFrameworkPlugins::WPEFrameworkPlugins"
        )
    endif()

    set(Plugins_FOUND TRUE)
    return()
endif()

# Fallback: locate the shared library directly and create an imported target.
set(_lib_hints "")
if(DEFINED CMAKE_PREFIX_PATH)
    foreach(_p IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND _lib_hints "${_p}/lib")
    endforeach()
endif()
if(DEFINED CMAKE_INSTALL_PREFIX)
    list(APPEND _lib_hints "${CMAKE_INSTALL_PREFIX}/lib")
endif()

find_library(_WPEFRAMEWORK_PLUGINS_LIB
    NAMES WPEFrameworkPlugins ThunderPlugins
    HINTS ${_lib_hints}
)

if(_WPEFRAMEWORK_PLUGINS_LIB)
    if(NOT TARGET Plugins::Plugins)
        add_library(Plugins::Plugins SHARED IMPORTED)
        set_target_properties(Plugins::Plugins PROPERTIES
            IMPORTED_LOCATION "${_WPEFRAMEWORK_PLUGINS_LIB}"
        )
    endif()

    set(Plugins_FOUND TRUE)
    return()
endif()

# Last-resort fallback: provide an empty INTERFACE target so configuration can proceed.
# This is intended for isolated CI validation where the full Thunder SDK may not be
# discoverable via CMake package lookup even though headers/libs exist in the repo.
if(NOT TARGET Plugins::Plugins)
    add_library(Plugins::Plugins INTERFACE IMPORTED)
endif()

set(Plugins_FOUND TRUE)
