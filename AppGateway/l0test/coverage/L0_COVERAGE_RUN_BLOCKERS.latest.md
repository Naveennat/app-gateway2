# L0 Coverage Run Blockers (latest)

Run date: 2026-01-02  
Run script: `app-gateway2/AppGateway/l0test/run_l0_coverage.sh`  
Output log: `app-gateway2/AppGateway/l0test/coverage/L0_COVERAGE_RUN_OUTPUT.latest.log`

## Blocker 1 — L0 test build fails at compile-time

**Target:** `appgateway_l0test`  
**First failing TU:** `Resolver_Configure_And_ResolveTests.cpp`

### Error
```
/home/kavia/workspace/code-generation/app-gateway2/AppGateway/l0test/Resolver_Configure_And_ResolveTests.cpp:193:56: error: template argument 1 is invalid
  , plugin(WPEFramework::Core::Service<AppGateway>::Create<IPlugin>())

/home/kavia/workspace/code-generation/app-gateway2/AppGateway/l0test/Resolver_Configure_And_ResolveTests.cpp:193:73: error: expected primary-expression before '>' token
/home/kavia/workspace/code-generation/app-gateway2/AppGateway/l0test/Resolver_Configure_And_ResolveTests.cpp:193:75: error: expected primary-expression before ')' token
```

### Location (in source)
The failure is around the `PluginAndService` constructor initialization list:

```
, plugin(WPEFramework::Core::Service<AppGateway>::Create<IPlugin>())
```

### Impact
Because the test binary does not compile, **no tests execute** and therefore **no coverage report** can be generated.

---

## Non-blocking warnings observed (not the root failure)

- Multiple instances of:
  - `-Woverloaded-virtual`: `PluginHost::IDispatcher::Invoke(...) was hidden`
  - Explicit constructor warning when using `{}` default config for `L0Test::ServiceMock::Config`

These warnings are present across multiple test files but **do not stop the build**. The compile error above is the immediate blocker.
