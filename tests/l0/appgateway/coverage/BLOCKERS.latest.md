# AppGateway L0 Coverage – Blockers (latest)

## What was run
- Script: `tests/l0/appgateway/run_coverage.sh`
- Plugin: `dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so`
- Thunder SDK prefix: `dependencies/install`
- Console output captured at: `tests/l0/appgateway/coverage/console.latest.txt`

## Primary blocker (runtime / harness interaction with installed plugin)
During L0 test execution the installed plugin fails to instantiate its implementation class:

- `Missing implementation classname AppGatewayImplementation in library`
- Plugin Initialize fails and returns: `Could not retrieve the AppGateway interface.`

This causes L0 tests to fail and then abort with an assertion:

- `ASSERT [plugin/AppGateway/AppGateway.cpp:65] (mResponder == nullptr)`
- Process terminates with: `Aborted (core dumped)` (exit code 134)

## Impact
Because the test process aborts before reaching the lcov/genhtml stage, the following artifacts were **not produced** by this run:
- `tests/l0/appgateway/coverage/coverage.info`
- `tests/l0/appgateway/coverage/html/`

## Notes
- Build completed; failures are at runtime when loading/instantiating the installed plugin implementation via Thunder's service instantiation.
- This task did **not** modify any plugin source files; only diagnostics/artifacts were captured/updated.
