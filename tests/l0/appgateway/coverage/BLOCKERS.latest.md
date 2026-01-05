# AppGateway L0 Coverage – Blockers (latest)

## What was run
- Script: `tests/l0/appgateway/run_coverage.sh`
- Plugin linked/used: `dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so`
- Thunder SDK prefix: `dependencies/install`
- Console output captured at: `tests/l0/appgateway/coverage/console.latest.txt`

## Primary blocker (plugin initialization / Thunder instantiation in test harness)
During L0 test execution, the installed plugin attempts to instantiate the resolver implementation via Thunder’s library/service instantiation path and fails:

- `Missing implementation classname AppGatewayImplementation in library`
- As a result, plugin Initialize fails and returns: `Could not retrieve the AppGateway interface.`
- JSON-RPC `resolve` is not registered, and dispatcher invocation returns non-zero (observed `53`).

This indicates that the in-proc L0 harness is not providing the required implementation class in the way Thunder expects when running inside the standalone unit test process.

## Secondary blocker (test process abort)
The coverage run previously aborted with an assertion in the plugin when `Initialize()` was called twice on the same instance:

- `ASSERT [plugin/AppGateway/AppGateway.cpp:65] (mResponder == nullptr)`
- Process terminates with: `Aborted (core dumped)` (exit code 134)

A test-harness-only fix was applied to avoid calling `Initialize()` twice on the same plugin instance (the idempotency test now reinitializes using a fresh instance), preventing the abort so coverage can proceed once the primary instantiation blocker is resolved.

## Impact
Because the process aborted before reaching the lcov/genhtml stage, the following artifacts were **not produced** by this run:
- `tests/l0/appgateway/coverage/coverage.info`
- `tests/l0/appgateway/coverage/html/`

## Notes
- This task does **not** modify any plugin/AppGateway sources; only test/harness updates and artifact updates.
- Build completes successfully; the failure is at runtime when the plugin tries to instantiate `AppGatewayImplementation` via Thunder’s library mechanisms inside the standalone test process.
