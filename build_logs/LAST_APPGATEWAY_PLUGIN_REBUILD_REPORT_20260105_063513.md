# AppGateway plugin rebuild report — 20260105_063513

## Command
Executed from `app-gateway2/`:
- `CI=true ./build_plugin_appgateway.sh | tee build_logs/rebuild_plugin_appgateway_console.log`

## Result marker
- `build_logs/RESULT_plugin_appgateway_20260105_063513.txt`
  - RESULT: **FAIL**
  - Timestamp: **20260105_063513**
  - Last tried C++ standard: **C++17**

## Logs captured
- Consolidated console log:
  - `build_logs/rebuild_plugin_appgateway_console.log`
- Configure log (coverage path):
  - `tests/l0/appgateway/coverage/build_plugin_appgateway_configure.log`
- Build log (coverage path):
  - `tests/l0/appgateway/coverage/build_plugin_appgateway_build.log`
- Full append log (timestamped by script):
  - `build_logs/build_plugin_appgateway_20260105_063513.log`

## Include path verification (normal include paths, no forced include)
From `build_plugin_appgateway_configure.log` / `CMakeCache.txt` dump:
- `APPGATEWAY_EXTRA_INCLUDE_DIRS:STRING=.../helpers/rdkservices-comcast/helpers;.../helpers/entservices-infra/helpers;.../helpers;.../interfaces`

This confirms `app-gateway2/interfaces/Ids.h` is reachable via include paths.

## Failure summary
Primary compile error indicates `Ids.h` is being pulled from two locations:
- `.../dependencies/install/include/WPEFramework/interfaces/Ids.h`
- `.../app-gateway2/interfaces/Ids.h`

This causes:
- `multiple definition of 'enum WPEFramework::Exchange::IDS'`

Additional errors (likely cascading from ID header collision/precedence):
- missing identifiers such as `ID_VALUE_POINT`, `ID_EXTERNAL`, `ID_STREAM`, `ID_PLAYER`, `ID_POWER`, etc., referenced by SDK headers under `dependencies/install/include/WPEFramework/interfaces/*`.
