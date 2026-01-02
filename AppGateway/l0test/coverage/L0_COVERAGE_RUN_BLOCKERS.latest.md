# L0 Coverage Run Blockers (latest)

## Goal
Run `AppGateway/l0test/run_l0_coverage.sh` using Thunder/WPEFramework **R4_4 / 4.4** from:
`/home/kavia/workspace/code-generation/dependencies/install`

## Current result
**FAIL** (link step): `appgateway_l0test` fails to link.

## Root cause
The test build compiles against the local interface header:
`app-gateway2/interfaces/IAppGateway.h` which defines:
- `WPEFramework::Exchange::GatewayContext`
- `IAppGatewayResolver::Configure(IStringIterator* const&)`
- `IAppGatewayResolver::Resolve(const GatewayContext&, const string&, const string&, const string&, string&)`

But the plugin `.so` being linked (`build/app-gateway/AppGateway/libWPEFrameworkAppGateway.so`) exports different signatures:
- `WPEFramework::Plugin::Resolver::Resolver()` and `Resolver(IShell*, const string&)` (no `Resolver(IShell*)`)
- `WPEFramework::Plugin::AppGatewayImplementation::Configure(PluginHost::IShell*)`
- `WPEFramework::Plugin::AppGatewayImplementation::Configure(WPEFramework::Exchange::IAppGateway::IStringIterator* const&)`
- `WPEFramework::Plugin::AppGatewayImplementation::Resolve(WPEFramework::Exchange::IAppGateway::Context const&, const string&, const string&)`

This indicates an **API mismatch** between what the L0 tests expect (GatewayContext + 5-arg Resolve) and what the built plugin actually implements (IAppGateway::Context + 3-arg Resolve).

## Evidence
From `coverage/L0_COVERAGE_RUN_LOGS.latest.txt`:
- `undefined reference to WPEFramework::Plugin::Resolver::Resolver(WPEFramework::PluginHost::IShell*)`
- `undefined reference to AppGatewayImplementation::Configure(RPC::IIteratorType<string,...>* const&)`
- `undefined reference to AppGatewayImplementation::Resolve(Exchange::GatewayContext const&, string const&, string const&, string const&, string&)`

## Next steps
Either:
1. Align the L0 tests to the plugin API in this repo (use `WPEFramework::Exchange::IAppGateway`/`Context` and the 3-arg `Resolve` signature), OR
2. Rebuild/link against a plugin variant that implements `IAppGatewayResolver` (GatewayContext + 5-arg Resolve) matching the interfaces used by tests.

Until the API mismatch is resolved, L0 coverage cannot complete.
