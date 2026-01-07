# CompileSettingsDebugConfig.cmake - compatibility shim
#
# Some components (like this repo's AppGateway plugin CMake) do:
#   find_package(CompileSettingsDebug REQUIRED)
#
# In some environments this package is provided by the Thunder SDK.
# In this repository's vendored SDK, CI validation can fail to discover it
# reliably. This shim provides a minimal package that supplies an interface
# target usable for compile options/definitions.
#
# If an upstream/real CompileSettingsDebugTargets.cmake exists, include it.
# Otherwise, define a minimal INTERFACE target.

cmake_minimum_required(VERSION 3.16)

# If a real targets export exists next to this file, prefer it.
set(_dir "${CMAKE_CURRENT_LIST_DIR}")
if(EXISTS "${_dir}/CompileSettingsDebugTargets.cmake")
    include("${_dir}/CompileSettingsDebugTargets.cmake")
endif()

# Provide a conventional target if none was defined by exports.
if(NOT TARGET CompileSettingsDebug::CompileSettingsDebug)
    add_library(CompileSettingsDebug::CompileSettingsDebug INTERFACE IMPORTED)

    # Best-effort defaults consistent with typical Thunder debug builds.
    # Keep minimal; consumers can add more as needed.
    set_target_properties(CompileSettingsDebug::CompileSettingsDebug PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "_THUNDER_DEBUG;_THUNDER_CALLSTACK_INFO"
        INTERFACE_COMPILE_OPTIONS "-std=c++11"
    )
endif()

set(CompileSettingsDebug_FOUND TRUE)
