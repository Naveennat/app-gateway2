# If not stated otherwise in this file or this component's license file the
# following copyright and licenses apply:
#
# Copyright 2020 RDK Management
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

set(PLUGIN_NAME DeviceBridge)
set(MODULE_NAME ${NAMESPACE}${PLUGIN_NAME})

find_package(${NAMESPACE}Plugins REQUIRED)

find_package(MDNS REQUIRED)
find_package(Boost COMPONENTS system filesystem REQUIRED)
find_package(websocketpp REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(IARMBus)
find_package(CURL)

add_library(${MODULE_NAME} SHARED
        ../helpers/MdnsUtils.cpp
        ../helpers/FileSystemNotification.cpp
        ../helpers/PolyM/src/Msg.cpp
        ../helpers/PolyM/src/Queue.cpp
        ../helpers/Processor.cpp
        ../helpers/UsbInterfaceMonitor.cpp
        ../helpers/WebSockets/Certificates.cpp
        ../helpers/WebSockets/SimpleWSClient.cpp
        ../helpers/WebSockets/WSEndpoint.cpp
        ../helpers/WebSockets/JsonRpc/Request.cpp
        ../helpers/WebSockets/JsonRpc/Response.cpp
        ../helpers/WebSockets/JsonRpc/Notification.cpp
        ../helpers/utils.cpp
        DeviceBridge.cpp
        DeviceBridgeUtils.cpp
        Module.cpp
        ConnectionManager.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_include_directories(${MODULE_NAME} PRIVATE ../helpers)
target_include_directories(${MODULE_NAME} PRIVATE ../helpers/WebSockets)
target_include_directories(${MODULE_NAME} PRIVATE ${MDNS_INCLUDE_DIR})
target_include_directories(${MODULE_NAME} PRIVATE ${IARMBUS_INCLUDE_DIRS})
target_include_directories(${MODULE_NAME} PRIVATE ${CURL_INCLUDE_DIRS})

target_link_libraries(${MODULE_NAME} PRIVATE ${NAMESPACE}Plugins::${NAMESPACE}Plugins)
target_link_libraries(${MODULE_NAME} PRIVATE ${MDNS_LIBRARY})
target_link_libraries(${MODULE_NAME} PRIVATE ${Boost_SYSTEM_LIBRARY})
target_link_libraries(${MODULE_NAME} PRIVATE OpenSSL::SSL)
target_link_libraries(${MODULE_NAME} PRIVATE OpenSSL::Crypto)
target_link_libraries(${MODULE_NAME} PRIVATE ${Boost_FILESYSTEM_LIBRARY})
target_link_libraries(${MODULE_NAME} PRIVATE ${IARMBUS_LIBRARIES})
target_link_libraries(${MODULE_NAME} PRIVATE ${CURL_LIBRARIES})

install(TARGETS ${MODULE_NAME} DESTINATION lib/${STORAGE_DIRECTORY}/plugins)

write_config(${PLUGIN_NAME})
