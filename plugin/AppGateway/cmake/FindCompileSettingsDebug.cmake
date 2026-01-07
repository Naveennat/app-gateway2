# FindCompileSettingsDebug.cmake - hard compatibility finder for this repo.
#
# The AppGateway plugin's CMake requires:
#   find_package(CompileSettingsDebug REQUIRED)
#
# In isolated CI validation environments, config-mode discovery can fail even if
# a vendored SDK exists. This module guarantees discovery by defining the
# expected target unconditionally.

if(NOT TARGET CompileSettingsDebug::CompileSettingsDebug)
    add_library(CompileSettingsDebug::CompileSettingsDebug INTERFACE IMPORTED)
    set_target_properties(CompileSettingsDebug::CompileSettingsDebug PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "_THUNDER_DEBUG;_THUNDER_CALLSTACK_INFO"
        INTERFACE_COMPILE_OPTIONS "-std=c++11"
    )
endif()

set(CompileSettingsDebug_FOUND TRUE)
