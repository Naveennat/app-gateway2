# AppGateway plugin build re-run report (2026-01-05)

## What was run
Re-ran:
- `app-gateway2/scripts/build_plugin_appgateway.sh`

This build uses the updated `plugin/AppGateway/CMakeLists.txt` that force-includes `interfaces/Ids.h` via:
- `target_compile_options(... "-include" "<repo>/interfaces/Ids.h")`

## Result
**FAILED** at compile stage.

## Logs/artifacts captured by the build script
All under:
- `app-gateway2/tests/l0/appgateway/coverage/`

Notable files:
- `build_plugin_appgateway_configure.log`
- `build_plugin_appgateway_build.log`
- `build_plugin_appgateway_errors.latest.txt`

## Failure signature (from build log)
The compiler command includes:
- `-std=gnu++11`
- `-include /home/kavia/workspace/code-generation/app-gateway2/plugin/AppGateway/../../interfaces/Ids.h`

But compilation fails parsing `Ids.h`:
- `/home/kavia/workspace/code-generation/app-gateway2/interfaces/Ids.h:51:14: error: found ‘:’ in nested-name-specifier, expected ‘::’`
- Line: `enum IDS : uint32_t {`

After that, `IAppGateway.h` reports undefined `ID_APP_GATEWAY*` identifiers (cascading due to the enum parse error).

## Next investigation pointers
- Verify why `/usr/bin/c++` (invoked with `-std=gnu++11`) rejects `enum IDS : uint32_t` syntax.
- Check whether `uint32_t` is in scope at the point `Ids.h` is forced-included (i.e., whether `<cstdint>` is available prior to parsing).
- Consider adjusting the force-include list to include a minimal stdint header before `Ids.h` if needed (e.g., `-include cstdint` or a repo shim header), without touching plugin sources.
