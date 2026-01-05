# Install script for directory: /home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces

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
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/wpeframework/proxystubs/libWPEFrameworkMarshalling.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/wpeframework/proxystubs/libWPEFrameworkMarshalling.so.1"
      )
    if(EXISTS "${file}" AND
       NOT IS_SYMLINK "${file}")
      file(RPATH_CHECK
           FILE "${file}"
           RPATH "")
    endif()
  endforeach()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/wpeframework/proxystubs" TYPE SHARED_LIBRARY FILES
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/interfaces/libWPEFrameworkMarshalling.so.1.0.0"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/interfaces/libWPEFrameworkMarshalling.so.1"
    )
  foreach(file
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/wpeframework/proxystubs/libWPEFrameworkMarshalling.so.1.0.0"
      "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/wpeframework/proxystubs/libWPEFrameworkMarshalling.so.1"
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/wpeframework/proxystubs" TYPE SHARED_LIBRARY FILES "/home/kavia/workspace/code-generation/app-gateway2/dependencies/build/ThunderInterfaces/interfaces/libWPEFrameworkMarshalling.so")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/interfaces" TYPE FILE FILES
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IAVNClient.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IAVSClient.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IAmazonPrime.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IAppManager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IApplication.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IBluetooth.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IBluetoothAudio.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IBrowser.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IButler.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ICapture.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ICommand.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IComposition.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ICompositionBuffer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IConfiguration.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IContentDecryption.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ICryptography.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ICustomerCareOperations.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDIALServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDRM.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDeviceInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDictionary.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDisplayInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDolby.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IDsgccClient.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IExternal.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IExternalBase.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IFocus.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IGuide.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IIPNetwork.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IInputPin.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IInputSwitch.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IKeyHandler.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ILISA.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ILanguageTag.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IMath.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IMediaPlayer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IMemory.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IMessageControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IMessenger.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/INetflix.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/INetflixSecurity.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/INetworkControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/INetworkTools.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IOCDM.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IPackageManager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IPackager.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IPerformance.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IPlayGiga.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IPlayerInfo.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IPower.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IProvisioning.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IRPCLink.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IRemoteControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IResourceMonitor.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IRtspClient.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IRustBridge.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IScriptEngine.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ISecureShellServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IStore.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IStore2.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IStoreCache.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IStream.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ISwitchBoard.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ISystemCommands.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ITestController.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ITestUtility.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ITextToSpeech.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ITimeSync.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/ITimeZone.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IUserSettings.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IValuePoint.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IVoiceHandler.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IVolumeControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IWatchDog.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IWebDriver.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IWebPA.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IWebServer.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IWifiControl.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/IZigWave.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/Ids.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/Portability.h"
    "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/Module.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/WPEFramework/interfaces/json" TYPE FILE FILES "/home/kavia/workspace/code-generation/app-gateway2/dependencies/ThunderInterfaces/interfaces/json/ExternalMetadata.h")
endif()

