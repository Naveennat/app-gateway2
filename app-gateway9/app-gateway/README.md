# AppGateway Thunder Plugin

This directory contains the implementation of the AppGateway plugin following the architecture described in:
- docs/appgateway-design.md
- docs/thunder-plugins-architecture.md

Current implementation provides:
- JSON-RPC methods: configure, resolve, respond
- Minimal internal stubs for Resolver, RequestRouter, ConnectionRegistry, PermissionManager, and GatewayWebSocket to keep the plugin buildable
- CMake configuration to build a shared library libWPEFrameworkAppGateway.so

Build:
- Ensure Thunder (WPEFramework) is available under dependencies/Thunder and WPEFramework CMake packages are discoverable.
- From this directory:
  mkdir -p build && cd build
  cmake .. -DWPEFramework_DIR=<path to Thunder cmake config if required>
  make -j

Install:
- Installs the library to lib/ and example configuration files under share/WPEFramework/AppGateway

Runtime:
- The .conf.in file illustrates how to load the plugin with initial configuration (serverPort, resolutionPaths, etc.).
- Methods are accessible under call sign org.rdk.AppGateway.1 (recommended), though the initial skeleton uses unversioned registration names as handlers and relies on Thunder JSONRPC to expose them.
