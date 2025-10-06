# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2025
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.10)

# Top-level project for the AppGateway component
project(AppGatewayProject LANGUAGES CXX)

# Thunder typically uses namespace WPEFramework; allow override from command line
if (NOT DEFINED NAMESPACE OR "${NAMESPACE}" STREQUAL "")
    set(NAMESPACE WPEFramework)
endif()

# Provide Thunder (WPEFramework) SDK install path hints so subdirectories can reuse them.
# This path resolves to: <repo-root>/dependencies/install
set(THUNDER_INSTALL_PATH "${CMAKE_SOURCE_DIR}/../dependencies/install" CACHE PATH "Path to Thunder (WPEFramework) install prefix" FORCE)
get_filename_component(THUNDER_INSTALL_PATH_ABS "${THUNDER_INSTALL_PATH}" REALPATH)
set(THUNDER_INCLUDE_DIR "${THUNDER_INSTALL_PATH_ABS}/include/${NAMESPACE}")
set(THUNDER_LIBRARY_DIR "${THUNDER_INSTALL_PATH_ABS}/lib")

# Enable CTest for the tests/ subdirectory
enable_testing()

# Build the AppGateway plugin, FbSettings plugin, and the unit tests
add_subdirectory(AppGateway)
add_subdirectory(FbSettings)
add_subdirectory(tests)
