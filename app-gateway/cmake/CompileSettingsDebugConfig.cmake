# Minimal fallback CompileSettingsDebug package config.
# Some environments build/install only Thunder headers/tools and do not ship the
# CMake package config files (CompileSettingsDebugConfig.cmake).
#
# This file provides a best-effort imported INTERFACE target matching the name
# expected by AppGateway/FbSettings/FbCommon: CompileSettingsDebug::CompileSettingsDebug

if (NOT TARGET CompileSettingsDebug::CompileSettingsDebug)
  add_library(CompileSettingsDebug::CompileSettingsDebug INTERFACE IMPORTED)

  # Conservative debug-ish flags; projects may add more as needed.
  set_target_properties(CompileSettingsDebug::CompileSettingsDebug PROPERTIES
    INTERFACE_COMPILE_OPTIONS "-g;-O0"
  )
endif()
