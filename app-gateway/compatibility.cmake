# Shared compatibility helpers injected by the validation wrapper before building the real plugin.
# Ensures the plugin build can locate repository-local Find*.cmake modules.
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/../plugin/AppGateway/cmake")

# Also extend CMAKE_PREFIX_PATH so config-mode find_package() can locate
# repo-local *Config.cmake shims if the plugin uses config-mode exclusively.
list(PREPEND CMAKE_PREFIX_PATH "${CMAKE_CURRENT_LIST_DIR}/../plugin/AppGateway/cmake")
set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" CACHE STRING "CMake prefix path (augmented for repo shims)" FORCE)
