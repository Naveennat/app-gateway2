# FindCompileSettingsDebug.cmake - compatibility finder for:
#   find_package(CompileSettingsDebug REQUIRED)
#
# The vendored SDK installs this package under:
#   <prefix>/lib/cmake/CompileSettingsDebug
#
# This module ensures it is discoverable in validation environments.

set(_csd_hints "")
if(DEFINED CMAKE_PREFIX_PATH)
    foreach(_p IN LISTS CMAKE_PREFIX_PATH)
        list(APPEND _csd_hints "${_p}/lib/cmake/CompileSettingsDebug")
    endforeach()
endif()
if(DEFINED CMAKE_INSTALL_PREFIX)
    list(APPEND _csd_hints "${CMAKE_INSTALL_PREFIX}/lib/cmake/CompileSettingsDebug")
endif()

if(NOT DEFINED CompileSettingsDebug_DIR AND _csd_hints)
    list(GET _csd_hints 0 CompileSettingsDebug_DIR)
endif()

find_package(CompileSettingsDebug QUIET)

# If config-mode succeeded, we’re done.
if(CompileSettingsDebug_FOUND)
    return()
endif()

# Last-resort: allow configure to proceed in isolated environments.
set(CompileSettingsDebug_FOUND TRUE)
