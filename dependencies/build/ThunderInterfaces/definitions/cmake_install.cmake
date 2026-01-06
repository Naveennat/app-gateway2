# Install script for directory: /home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/definitions

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/home/kavia/workspace/code-generation/app-gateway2/dependencies/install")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libs" OR NOT CMAKE_INSTALL_COMPONENT)
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkDefinitions.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkDefinitions.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/libWPEFrameworkDefinitions.so.1.0.0"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/libWPEFrameworkDefinitions.so.1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkDefinitions.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libWPEFrameworkDefinitions.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHANGE
           FILE "${file}"
           OLD_RPATH "/home/kavia/workspace/code-generation/app-gateway2/dependencies/install/lib:"
           NEW_RPATH "")
      if(CMAKE_INSTALL_DO_STRIP)
        execute_process(COMMAND "/usr/bin/strip" "${file}")
      endif()
    endif()
  endforeach()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "libs" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/libWPEFrameworkDefinitions.so")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/definitions" TYPE FILE FILES
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/definitions/definitions.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/definitions/ValuePoint.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/Module.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/interfaces/json" TYPE FILE FILES
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JAVSController.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JAmazonPrime.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JAppManager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JApplication.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBluetoothAudioSink.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBluetoothControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBluetoothRemoteControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBrowser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBrowserCookieJar.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBrowserResources.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBrowserScripting.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JBrowserSecurity.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JButler.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JCobalt.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JCompositor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JConnectionProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JContainers.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JCustomerCareOperations.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDHCPServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDIALServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDTV.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDeviceIdentification.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDeviceInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDisplayInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDisplayProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JDolbyOutput.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JFirmwareControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JGraphicsProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JHDRProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JIOConnector.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JIOControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JInputSwitch.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JLISA.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JLanguageTag.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JLocationSync.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JMath.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JMessageControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JMessenger.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JMonitor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JNetflix.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JNetworkControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JNetworkTools.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JOCDM.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPackageManager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPackager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPerformanceMonitor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPersistentStore.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPlayerInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPlayerProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JPower.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JProvisioning.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JRemoteControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JScriptEngine.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JSecureShellServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JSecurityAgent.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JSpark.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JStateControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JStreamer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JSubsystemControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JSystemCommands.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JTestController.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JTestUtility.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JTimeSync.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JTimeZone.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JTraceControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JUserSettings.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JVolumeControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JWatchDog.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JWebBrowser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JWebKitBrowser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JWifiControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JZigWave.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_AVSController.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_AppManager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Application.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BluetoothAudioSink.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BluetoothControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BluetoothRemoteControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Browser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BrowserCookieJar.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BrowserResources.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BrowserScripting.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_BrowserSecurity.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Butler.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Compositor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_ConnectionProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Containers.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DHCPServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DIALServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DTV.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DeviceIdentification.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DeviceInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DisplayInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DisplayProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_DolbyOutput.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_FirmwareControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_HDRProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_IOConnector.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_IOControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_InputSwitch.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_LISA.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_LocationSync.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Math.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_MessageControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Messenger.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Monitor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Netflix.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_NetworkControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_NetworkTools.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_OCDM.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_PackageManager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Packager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_PerformanceMonitor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_PersistentStore.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_PlayerInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_PlayerProperties.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Power.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Provisioning.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_RemoteControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_ScriptEngine.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_SecureShellServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_SecurityAgent.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_StateControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_Streamer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_SubsystemControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_SystemCommands.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_TestController.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_TestUtility.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_TimeSync.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_TraceControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_UserSettings.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_VolumeControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_WatchDog.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_WebBrowser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_WebKitBrowser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_WifiControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/JsonData_ZigWave.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/Module.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/generated/Ids.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/WPEFrameworkDefinitions.pc")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions/WPEFrameworkDefinitionsTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions/WPEFrameworkDefinitionsTargets.cmake"
         "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/CMakeFiles/Export/f90844c33684d701a622957b301a5338/WPEFrameworkDefinitionsTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions/WPEFrameworkDefinitionsTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions/WPEFrameworkDefinitionsTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions" TYPE FILE FILES "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/CMakeFiles/Export/f90844c33684d701a622957b301a5338/WPEFrameworkDefinitionsTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions" TYPE FILE FILES "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/CMakeFiles/Export/f90844c33684d701a622957b301a5338/WPEFrameworkDefinitionsTargets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/WPEFrameworkDefinitions" TYPE FILE FILES
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/WPEFrameworkDefinitionsConfigVersion.cmake"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/definitions/WPEFrameworkDefinitionsConfig.cmake"
    )
endif()

