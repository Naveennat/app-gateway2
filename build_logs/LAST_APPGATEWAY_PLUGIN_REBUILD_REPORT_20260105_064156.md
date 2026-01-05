# AppGateway plugin rebuild report (install prefix updated)

## Summary
- Status: **OK**
- Timestamp: **20260105_064156**
- Install prefix: `/home/kavia/workspace/code-generation/app-gateway2/dependencies/install`
- Build script: `app-gateway2/build_plugin_appgateway.sh`
- C++ standard used: **C++11**
- Build dir:
  - `/home/kavia/workspace/code-generation/app-gateway2/build/plugin_appgateway_20260105_064156_cxx11`

## Installed artifacts (under local prefix)
- Plugin shared library:
  - `/home/kavia/workspace/code-generation/app-gateway2/dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so`
- Plugin data/config installed by this build:
  - `/home/kavia/workspace/code-generation/app-gateway2/dependencies/install/etc/app-gateway/resolution.base.json`
  - `/home/kavia/workspace/code-generation/app-gateway2/dependencies/etc/WPEFramework/plugins/AppGateway.json`
    - Note: this comes from `dependencies/install/../etc/...` path expansion seen during `install`.

## Logs captured
- Result file:
  - `app-gateway2/build_logs/RESULT_plugin_appgateway_20260105_064156.txt`
- Timestamped build log:
  - `app-gateway2/build_logs/build_plugin_appgateway_20260105_064156.log`
- Configure/build logs (coverage dir):
  - `app-gateway2/tests/l0/appgateway/coverage/build_plugin_appgateway_configure.log`
  - `app-gateway2/tests/l0/appgateway/coverage/build_plugin_appgateway_build.log`

## Notable output snippets
- `cmake --install` output includes:
  - `-- Installing: .../dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so`
  - `-- Installing: .../dependencies/install/etc/app-gateway/resolution.base.json`
  - `-- Installing: .../dependencies/install/../etc/WPEFramework/plugins/AppGateway.json`
- CMake warning:
  - `Manually-specified variables were not used by the project: APPGATEWAY_SYSCONFDIR`
