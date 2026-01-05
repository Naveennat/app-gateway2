# Fallback helpers for Thunder/WPEFramework plugin builds in minimal environments.
#
# Thunder-based plugin projects often rely on helper macros like `write_config()`
# which are usually provided by Thunder's CMake modules. Some CI environments
# (or partial SDK installs) may not include those modules. This file provides
# a no-op compatible fallback to unblock configuration/build.

# PUBLIC_INTERFACE
function(write_config)
  # Fallback implementation of write_config().
  # Accept any arguments but intentionally do nothing.
  # Real Thunder environments generate plugin JSON configs here.
  message(STATUS "[fallback] write_config(${ARGV}) (no-op)")
endfunction()
