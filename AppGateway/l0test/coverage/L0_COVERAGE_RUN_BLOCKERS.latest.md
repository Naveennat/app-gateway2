# L0 Coverage Run Blockers (latest)

## Goal
Run `AppGateway/l0test/run_l0_coverage.sh` using Thunder/WPEFramework **R4_4 / 4.4** from:
`/home/kavia/workspace/code-generation/dependencies/install`

## SDK Alignment (Thunder 4.4)
- `dependencies/install/lib/cmake/WPEFramework/WPEFrameworkConfig.cmake` exists.
- `dependencies/install/include/WPEFramework/core/Portability.h` defines `THUNDER_VERSION 4`.
- `dependencies/install/include/WPEFramework/core/JSONRPC.h` contains Thunder 4.4 compatibility notes.

## Current blocker
The L0 coverage run fails at **compile time** in:
`AppGateway/l0test/Resolver_Configure_And_ResolveTests.cpp`

Error (from `coverage/L0_COVERAGE_RUN_LOGS.latest.txt`):
- `template argument 1 is invalid` at the plugin creation expression.

## What was changed in this step
- Forced SDK resolution via:
  - `CMAKE_PREFIX_PATH=/home/kavia/workspace/code-generation/dependencies/install`
  - `PKG_CONFIG_PATH=/home/kavia/workspace/code-generation/dependencies/install/lib/pkgconfig`
  in `run_l0_coverage.sh`.
- Updated `Resolver_Configure_And_ResolveTests.cpp` to instantiate the plugin using:
  `WPEFramework::Core::Service<WPEFramework::Plugin::AppGateway>::Create<WPEFramework::PluginHost::IPlugin>()`
  pattern (Thunder 4.x compatible), rather than an invalid `Core::Service<AppGateway>::Create<IPlugin>()`.

## Artifacts
- Build+run logs: `AppGateway/l0test/coverage/L0_COVERAGE_RUN_LOGS.latest.txt`
- Result marker (updated by script when successful): `AppGateway/l0test/coverage/L0_COVERAGE_RUN_RESULT.latest.txt`
- Coverage HTML output (when successful): `AppGateway/l0test/coverage/index.html`

## Next expected outcome
After these changes, rerunning `./run_l0_coverage.sh` should:
1) Configure/build against Thunder 4.4 SDK from `dependencies/install`
2) Compile `appgateway_l0test`
3) Execute tests
4) Generate `coverage/coverage.info` and `coverage/index.html`
