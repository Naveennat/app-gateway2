# Local fallback config for find_package(CompileSettingsDebug).
# This file exists to satisfy config-mode package lookup in isolated repo builds.

cmake_minimum_required(VERSION 3.16)

if(NOT TARGET CompileSettingsDebug::CompileSettingsDebug)
    add_library(CompileSettingsDebug::CompileSettingsDebug INTERFACE IMPORTED)
    set_target_properties(CompileSettingsDebug::CompileSettingsDebug PROPERTIES
        INTERFACE_COMPILE_DEFINITIONS "_THUNDER_DEBUG;_THUNDER_CALLSTACK_INFO"
        INTERFACE_COMPILE_OPTIONS "-std=c++11"
    )
endif()

set(CompileSettingsDebug_FOUND TRUE)
