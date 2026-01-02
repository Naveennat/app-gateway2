# L0 Coverage Run - Latest Blockers (auto note)

This run executed:

- Script: `AppGateway/l0test/run_l0_coverage.sh`
- Expected SDK prefix (script output): `/home/kavia/workspace/code-generation/dependencies/install`

## Result
Build failed at compile-time.

## First failing compile
Header errors from:
- `/home/kavia/workspace/code-generation/dependencies/install/include/WPEFramework/plugins/IDispatcher.h`

Key error:
- `error: ‘string’ does not name a type`

## Additional missing SDK surface
AppGateway plugin compilation also requires:
- `WPEFramework::PluginHost::IPlugin`
- `WPEFramework::PluginHost::JSONRPC`
- Interface map macros: `BEGIN_INTERFACE_MAP`, `INTERFACE_ENTRY`, `INTERFACE_AGGREGATE`, `END_INTERFACE_MAP`

These are not currently provided by:
- `/home/kavia/workspace/code-generation/dependencies/install/include/WPEFramework/plugins/plugins.h`

## Raw log capture location used by agent
- `/tmp/l0_cov.out` (stdout+stderr from the script run)
- `/tmp/ninja_build.out` (verbose ninja build attempt)

## Once build succeeds, expected coverage artifacts
The script indicates output under:
- `app-gateway2/AppGateway/l0test/coverage/`
