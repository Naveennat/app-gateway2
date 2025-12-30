# L0/L1/L2 Testing Design

This document supersedes l1_l2_testing_design.md; use this file going forward.

## Overview

This document describes how the `AppGateway` plugin is tested at three levels: L0 (offline and deterministic), L1 (component/unit testing with shared mocks), and L2 (integration-style tests executed through a Thunder/WPEFramework test plugin). It focuses on what is implemented in this repository today, and it uses concrete code references from the existing L0 test harness and the L2 test controller/plugin.

The key idea across all levels is to keep tests repeatable by controlling the plugin host environment. For L0, this is done by running the plugin in-process with a custom `IShell`/`ICOMLink` mock. For L2, the repository provides a controller that starts a local `WPEFramework` process and invokes `RUN_ALL_TESTS()` through a dedicated Thunder plugin.

## L0 vs L1 vs L2 comparison (AppGateway-specific)

The table below compares the three levels as they are implemented (or intended to be implemented) in this repository. It is written to help you quickly decide which level to use, what you need running on your machine, and what kind of failures each level tends to catch.

| Aspect | L0 (offline / deterministic) | L1 (unit/component with shared mocks) | L2 (integration via Thunder test plugin) |
|---|---|---|---|
| Purpose | L0 is meant to validate AppGateway behavior in-process with deterministic fakes, with an emphasis on lifecycle correctness, JSON-RPC registration, and predictable error-path coverage. In this repo, L0 is the fastest way to prove that “resolve” routing and core return codes work. | L1 is meant to validate component-level behavior using GoogleTest and GoogleMock against the same interfaces used in production, but with mocks supplied by the shared `entservices-testframework`. It is most useful when you want expectation-based testing and richer assertions than L0’s direct return-code checks. | L2 is meant to validate higher-level integration by running GoogleTest suites from inside a Thunder plugin. In this repo, L2 uses a controller that boots a local `WPEFramework` process and then calls a JSON-RPC method (`PerformL2Tests`) on the `org.rdk.L2Tests.1` plugin, which runs `RUN_ALL_TESTS()`. |
| Scope | AppGateway plugin lifecycle (`Initialize()`/`Deinitialize()`), JSON-RPC dispatcher registration for `"resolve"`, and deterministic resolver/responder behaviors via in-proc fakes. This level is explicitly designed to avoid any Thunder daemon, networking, or external services. | Expected to cover internal module interactions with mock expectations, typically within plugin boundaries. In this repo, the “L1 story” is primarily represented by the presence of the shared mocks and the testframework guidance, rather than a documented AppGateway-specific L1 runner. | End-to-end “bring up Thunder + run gtest suites” flow, plus environment orchestration such as controlling plugin autostart. L2 is closer to production execution because tests run through Thunder plugin infrastructure and JSON-RPC wiring. |
| Dependencies (frameworks, mocks) | Uses the AppGateway L0 harness under `app-gateway2/app-gateway/AppGateway/l0test/`. The harness supplies `L0Test::ServiceMock` and deterministic fakes (`ResolverFake`, `ResponderFake`) implemented in `app-gateway2/app-gateway/AppGateway/l0test/ServiceMock.h`. This level does not rely on GoogleTest/GoogleMock. | Uses GoogleTest/GoogleMock and the shared mock/stub inventory under `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/Tests/mocks/` (for example, Thunder/COMLink-related mocks under `Tests/mocks/thunder/`). The testframework README explains the multi-repo model where mocks are centralized and per-plugin tests are built as test libraries. | Uses GoogleTest/GoogleMock inside the `L2Tests` Thunder plugin (`app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/L2TestsPlugin/L2Tests.cpp`, `L2Tests.h`). The L2 stack also depends on the controller (`Tests/L2Tests/L2testController.cpp`) and may optionally involve additional scaffolding such as Pact provider state tooling (see below). |
| Runtime requirements (daemon/processes) | No daemon is required. Tests run in one process as `appgateway_l0test`. The harness emulates `PluginHost::IShell` and `IShell::ICOMLink` so `IShell::Root<T>()` can return in-proc fakes rather than spawning COM-RPC processes. | Typically no Thunder daemon is required for L1 if tests are truly component-level, but the environment can become more complex depending on what is being mocked. The shared testframework approach is frequently orchestrated through CI workflows and may not have a “single command” local runner within this repository. | Requires launching a `WPEFramework` process. In this repo that is done programmatically by the controller via `popen()`, not manually. The controller exports `THUNDER_ACCESS=127.0.0.1:<THUNDER_PORT>` and then invokes the test plugin over JSON-RPC. For Pact/provider-state-driven L2 runs, an additional Flask process may be used (see provider state service below). |
| Typical inputs / outputs | Typical input is a JSON parameter blob passed to the dispatcher’s `Invoke(..., "resolve", ...)` call; output is a Thunder `Core::hresult` (as a `uint32_t`) plus a response string containing JSON (often `"null"` in the current fakes). | Typical input is C++ test fixtures and mock expectations; output is standard gtest console output (and whatever artifact format your runner config emits). | Typical input is a JSON-RPC call to `org.rdk.L2Tests.1` method `"PerformL2Tests"` with optional `"test_suite_list"` for filter control; output is the gtest status code and a JSON report file `rdkL2TestResults.json` (via `GTEST_OUTPUT="json:$PWD/rdkL2TestResults.json"` as set in the controller). |
| Example scenarios in this repo | AppGateway lifecycle success/failure under missing dependency conditions; registration/unregistration of `"resolve"`; deterministic mapping of methods to error codes such as `"l0.notSupported"` → `Core::ERROR_NOT_SUPPORTED` and `"l0.notAvailable"` → `Core::ERROR_UNAVAILABLE`; responder “transport unavailable” injection via `ResponderFake`. | Using shared mocks (for example COMLink, device settings, IARM, RBUS, etc.) to validate interface contract behavior, call ordering, and error handling without needing the full runtime. The mock inventory in this repo is extensive (see `entservices-testframework/Tests/mocks/`), even when a specific AppGateway L1 suite is not documented here. | “Boot Thunder locally + run all enabled gtest suites” via the L2Tests plugin. The controller also demonstrates a concrete stability measure: it rewrites plugin JSON configs under `./install/etc/WPEFramework/plugins/` to set `"autostart":false` for everything except `L2Tests.json`, reducing the chance of crashes from plugins doing early IARM calls before mocks are ready. |
| Pros | Extremely fast and deterministic. Failures usually point directly to AppGateway logic or JSON boundary issues. This level is well suited to tight development loops and pre-submit checks. | Rich assertion model and expectation-based validation via gtest/gmock. Centralized mocks reduce duplication across repos and can keep platform-interface test seams consistent. | Catches issues that depend on process-level wiring (Thunder startup, plugin activation order, JSON-RPC integration, environment configuration) and is closer to how the system behaves in deployment. |
| Cons | Does not validate the real Thunder runtime environment, plugin activation semantics, or real networking. Because dependencies are faked, it can miss integration failures that only show up when running under `WPEFramework`. | In practice, the centralized testframework model can make local execution less straightforward, because L1 tests may be built as per-repo shared libraries and executed through higher-level orchestration. Developers may need to follow CI-like steps to reproduce locally. | Slower and more environment-sensitive. Failures can be caused by plugin startup ordering, port conflicts, missing runtime assets/config, or incomplete isolation of the local `install/etc/WPEFramework/plugins` environment. |
| Speed / determinism notes | Fastest and most deterministic because no process is spawned and no I/O is required. Tests are repeatable as long as the build output is stable. | Generally fast, but heavier than L0 due to gtest/gmock and broader mock graphs. Determinism depends on thread scheduling and how mocks interact with background worker pools. | Slowest due to process orchestration and potential external scaffolding. Determinism depends on controlling the Thunder environment (including which plugins autostart) and any auxiliary services used by tests. |
| Coverage notes | L0 is well suited to producing high line/branch coverage of AppGateway logic that is reachable in-process. The repo provides explicit L0 coverage commands using `lcov` and `genhtml`. | Coverage collection depends on how the L1 binaries/libraries are built and executed. The shared testframework model can support coverage, but it is not described as a single, AppGateway-local command in this repository. | Coverage is possible if built with coverage flags, but the visible L2 harness is primarily focused on producing gtest JSON output (`rdkL2TestResults.json`). |
| When to use | Use L0 when you are iterating on resolver/responder behavior, error codes, JSON parsing/serialization boundaries, and registration semantics, and you want quick feedback with minimal environment setup. | Use L1 when you need expectation-level verification across interfaces, want gtest fixtures and matchers, and want to leverage the shared mock ecosystem from `entservices-testframework`. | Use L2 when you need to validate the “realistic” execution model (Thunder + plugin + JSON-RPC), when failures only show up under process orchestration, or when verifying suites in the same way CI-like runners invoke them. |
| Example command snippets (build / run / coverage) | Build (repo root): `cmake -G Ninja -S app-gateway2/app-gateway -B build/appgatewayl0test -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$PWD/dependencies/install -DCMAKE_INSTALL_PREFIX=$PWD/dependencies/install`<br/>Build target: `cmake --build build/appgatewayl0test --target appgateway_l0test -j`<br/>Run: `export LD_LIBRARY_PATH=$PWD/build/appgatewayl0test/AppGateway:$PWD/dependencies/install/lib:$LD_LIBRARY_PATH` then `./build/appgatewayl0test/AppGateway/appgateway_l0test`<br/>Coverage: `lcov -c -o coverage.info -d build/appgatewayl0test` then `genhtml -o coverage coverage.info` (or `bash app-gateway2/app-gateway/scripts/appgateway_l0_coverage.sh build/appgatewayl0test`) | The testframework README describes a CI-like local run pattern using `act` (in an entservices-* repo context): `./bin/act -W .github/workflows/tests-trigger.yml -s GITHUB_TOKEN=<your access token>`. In this repository, the concrete, visible assets are the shared mocks and testframework structure under `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/`, rather than an AppGateway-specific L1 runner command. | Run the controller executable built from `Tests/L2Tests/L2testController.cpp`. The controller starts Thunder with a command like: `export GTEST_OUTPUT="json:$PWD/rdkL2TestResults.json"; WPEFramework -c <CMAKE_INSTALL_PREFIX>/../etc/WPEFramework/config.json` and sets `THUNDER_ACCESS` to `127.0.0.1:<THUNDER_PORT>` (default port string is `"9998"`). It then calls JSON-RPC method `PerformL2Tests` on callsign `org.rdk.L2Tests.1`.<br/>Optional filtering: pass suite names as CLI args; the controller builds a gtest filter like `SuiteA*:SuiteB*` and sends it as `"test_suite_list"`. |

## Rationale: why L0 does not reuse the L1/L2 Thunder mocks

This repository contains two intentionally distinct mocking strategies.

L0 (under `app-gateway2/app-gateway/AppGateway/l0test/`) uses a purpose-built, deterministic in-process host (`L0Test::ServiceMock`) that returns concrete fakes (`ResolverFake`, `ResponderFake`) and runs without GoogleTest/GoogleMock. L1/L2 (under `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/`) uses a shared gtest/gmock ecosystem (including “Thunder mocks”) aimed at expectation-based tests and at being executed through the central testframework runner (and, for L2, through a Thunder plugin harness).

Although it is tempting to “reuse the L1/L2 Thunder mocks in L0”, doing so would undermine the primary value of L0 (offline determinism and minimal dependencies) while still not providing all AppGateway-specific behaviors that the current L0 harness depends on (notably the way dependencies are injected through COMLink instantiation).

### 1) Different test philosophy and dependencies

L0 tests are designed to be lightweight, fast, and deterministic. They are built as a standalone executable (`appgateway_l0test`) and use a minimal assertion style (see `ExpectEqU32` / `ExpectEqStr` patterns in `app-gateway2/app-gateway/AppGateway/l0test/AppGatewayTest.cpp`). This level focuses on return codes, JSON boundary handling, and JSON-RPC registration/unregistration and is deliberately not structured as a gtest/gmock suite.

L1/L2 tests, by contrast, are designed around GoogleTest/GoogleMock. The shared Thunder mocks (for example, `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/Tests/mocks/thunder/ServiceMock.h`) are meant to be driven via expectations and fixtures, and they naturally pull in additional test tooling, build rules, and runtime conventions.

### 2) AppGateway-specific instantiation logic vs. generic mocks

AppGateway L0 is not merely “mocking IShell”. The L0 harness emulates a specific Thunder host mechanism: how `IShell::Root<T>()` ultimately depends on `IShell::ICOMLink::Instantiate(...)` to obtain interfaces at initialization time.

In `app-gateway2/app-gateway/AppGateway/l0test/ServiceMock.h`, `L0Test::ServiceMock` implements both `WPEFramework::PluginHost::IShell` and `WPEFramework::PluginHost::IShell::ICOMLink`. It provides a deterministic `Instantiate(...)` that is AppGateway-aware:

- Instantiate call #1 returns an `Exchange::IAppGatewayResolver` (backed by `ResolverFake`).
- Instantiate call #2 returns an `Exchange::IAppGatewayResponder` (backed by `ResponderFake`).
- It can return `nullptr` for either dependency to simulate missing dependencies and validate initialization failure and “resolve not registered” behavior.

The shared gmock-based Thunder `ServiceMock` used by L1/L2 primarily mocks the `IShell` surface and is intentionally generic so it can support many plugins. It is not designed to be an AppGateway-specific dependency injector, and on its own it does not provide L0’s concrete COMLink instantiation behavior.

### 3) Version/signature compatibility risks

Reusing L1/L2 Thunder mocks in L0 would introduce a compatibility surface with Thunder-version-specific signatures and build switches.

The gmock-based Thunder `ServiceMock` in `entservices-testframework` has conditional compilation branches (for example, `#ifdef USE_THUNDER_R4`) that change which methods exist and what signatures they have. This flexibility is useful for a shared framework, but it increases the risk of subtle mismatches if pulled into the minimal L0 build.

L0, in contrast, is intentionally aligned to the “installed” Thunder headers used by this repository build (for example, headers under `dependencies/install/include/WPEFramework/...`) and avoids multi-version mock shims.

### 4) Build/runtime footprint

L0 is intended to be a cheap inner-loop test. Pulling L1/L2 gmock infrastructure into L0 would increase build time, link time, and binary size, and it would add more places where environment or dependency issues can break the fast L0 signal.

In practice, this would make L0 less useful as a “compile quickly, run quickly, diagnose quickly” feedback loop.

### 5) What the L0 `ServiceMock` provides that L1/L2 mocks don’t

The L0 `ServiceMock` is not a generic mock; it is a small deterministic host emulator. Its value is that it provides behavior, not just stubs:

It provides in-process dependency injection via `IShell::ICOMLink::Instantiate(...)`, enabling AppGateway initialization without spawning COM-RPC processes while preserving the plugin’s natural “Root<T>()” dependency pattern.

It provides deterministic “domain fakes” (not expectation-only mocks). `ResolverFake::Resolve(...)` implements stable error mappings based on the requested method name (for example `l0.notPermitted`, `l0.notSupported`, and `l0.notAvailable`) so L0 tests can cover error paths without filesystem or network dependencies.

It provides test-facing behavior controls that L0 specifically needs, such as responder “transport available/unavailable” toggling and minimal notification/context scaffolding for responder behaviors.

It is designed to work with L0’s explicit runtime bootstrap. The L0 test executable bootstraps `Core::WorkerPool` and optionally a no-op `Core::Messaging::IStore` in `AppGatewayTest.cpp` so that in-proc Thunder components used by the plugin do not abort. This is consistent with the “host emulator” approach.

### 6) L0 test call flow (sequence diagram)

The diagram below shows the typical L0 call flow when testing `resolve`. The key detail is that the test process supplies the “host” (`L0Test::ServiceMock`) and provides the resolver/responder fakes by implementing COMLink instantiation.

```mermaid
sequenceDiagram
  participant T as "appgateway_l0test"
  participant B as "L0TestBootstrap"
  participant P as "AppGateway plugin (IPlugin)"
  participant S as "L0Test::ServiceMock (IShell and ICOMLink)"
  participant I as "ICOMLink::Instantiate"
  participant R as "ResolverFake"
  participant E as "ResponderFake"
  participant D as "IDispatcher"

  T->>B: "Initialize WorkerPool and Messaging (test-only)"
  T->>P: "Create plugin instance"
  T->>S: "Create ServiceMock (Config)"
  T->>P: "Initialize(S)"

  P->>S: "COMLink()"
  S->>I: "Instantiate call #1"
  I-->>P: "IAppGatewayResolver (ResolverFake)"

  P->>S: "COMLink()"
  S->>I: "Instantiate call #2"
  I-->>P: "IAppGatewayResponder (ResponderFake)"

  P->>P: "Register JSON-RPC method resolve"
  T->>P: "QueryInterface(IDispatcher)"
  P-->>T: "Return dispatcher"

  T->>D: "Invoke(resolve, paramsJson)"
  D->>R: "Resolve(...)"
  R-->>D: "Return Core::hresult and JSON payload"
  D-->>T: "Return rc and response string"

  T->>P: "Deinitialize(S)"
```

### 7) Could L1/L2 mocks be integrated into L0?

It is technically possible, but it is not recommended for this repository’s current goals.

Integrating L1/L2 Thunder mocks into L0 would effectively turn L0 into a gtest/gmock-based environment (i.e., “a small L1”), because the shared mocks are designed to be driven by expectations and fixtures. In concrete terms, it would require:

You would need to add gtest/gmock as build/link dependencies for L0, and restructure the L0 executable (and its tests) around gtest fixtures and gmock expectations. This materially changes the purpose of L0.

You would need an adapter that provides the COMLink instantiation behavior AppGateway needs. L0 requires `IShell::ICOMLink::Instantiate(...)` to return real objects implementing `IAppGatewayResolver` and `IAppGatewayResponder`. The generic L1/L2 `ServiceMock` focuses on mocking the `IShell` surface and does not, by itself, supply this AppGateway-specific injection behavior.

You would need to manage Thunder signature variability (for example `USE_THUNDER_R4`) and ensure that the L0 build is compatible with those conditional definitions. That adds a new compatibility matrix to what is currently a deliberately small test surface.

### 8) Better pattern

A better pattern is to keep L0 as-is and treat L0 and L1/L2 as complementary rather than trying to unify them through shared mocks.

If code reuse is desirable, it should focus on sharing domain fakes (deterministic, dependency-free behavior) rather than sharing test harness mocks. The L0 “host emulator” (`L0Test::ServiceMock`) should remain L0-specific because it encodes the execution model L0 relies on, while fakes that encode domain logic can be shared when appropriate.

### 9) Bottom line

L0 does not reuse the L1/L2 Thunder mocks because L0 is intentionally a different kind of test. It is an offline, deterministic, minimal-dependency in-process harness that emulates the specific host behaviors AppGateway needs (notably COMLink instantiation) and uses concrete deterministic fakes. L1/L2 is a gtest/gmock ecosystem built for expectation-based tests and broader shared-mock reuse.

Reusing L1/L2 mocks in L0 would increase complexity and coupling, introduce Thunder-version signature risks, enlarge build/runtime footprint, and still not automatically provide AppGateway’s required dependency injection behavior without additional adapters.

### 10) Instructions for future agent

If the goal is to reduce duplication without collapsing the test levels, the practical approach is to introduce a tiny shared “domain fakes” library that does not depend on gtest/gmock, and to optionally add a gmock adapter layer in L1/L2.

The shared “domain fakes” library should contain only deterministic fakes and helpers (for example, a reusable resolver fake and responder fake behavior) and should have stable interfaces and minimal dependencies. L0 can keep using `L0Test::ServiceMock::Instantiate(...)` to construct those fakes in-process, preserving L0’s execution model.

Optionally, L1/L2 can provide a thin adapter that either wraps those fakes with gmock expectations or delegates default behavior to the fake while still allowing expectations to be set. The key requirement is that gmock remains confined to L1/L2 builds, while the domain fakes remain usable by L0 and remain stable across test levels.

## Architecture

### L0 (offline, in-process)

AppGateway L0 tests live under `app-gateway2/app-gateway/AppGateway/l0test/` and build an `appgateway_l0test` executable. This test binary exercises the AppGateway plugin logic entirely in-process, and it avoids starting a Thunder daemon or creating real network connections.

The L0 harness is built around `L0Test::ServiceMock`, which implements:

- `WPEFramework::PluginHost::IShell` so the plugin can call `Initialize()` and `Deinitialize()` normally.
- `WPEFramework::PluginHost::IShell::ICOMLink` so that `IShell::Root<T>()` calls inside the plugin can resolve into deterministic in-process fakes instead of spawning out-of-process COM-RPC components.

In `app-gateway2/app-gateway/AppGateway/l0test/ServiceMock.h`, `ServiceMock::Instantiate(...)` returns the fakes based on call order, which matches the plugin initialization pattern. The first instantiation returns a `ResolverFake` (implementing `Exchange::IAppGatewayResolver`), and the second returns a `ResponderFake` (implementing `Exchange::IAppGatewayResponder`).

### L1 (unit/component tests with centralized mocks)

This repository includes `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/`, which documents the broader RDK approach where gtest/gmock-based stubs are centralized in a shared “testframework” to avoid duplication. The README in this folder explains the intended build job split:

- Build mocks into a `TestMock` library.
- Build per-repository test shared libraries.
- Build the testframework executable(s) that link those test libraries.

Within this repo, the most visible, concrete implementations are the L2 controller and L2 test plugin described below. The L0 tests are explicitly not gtest-based, and instead use a minimal custom assertion style.

### L2 (integration-style tests via a Thunder plugin)

L2 tests are orchestrated in two pieces:

- A controller executable: `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/L2testController.cpp`
- A Thunder test plugin: `app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/L2TestsPlugin/L2Tests.cpp`

The controller starts a local `WPEFramework` process using `popen()`, sets up the JSON-RPC access point using the `THUNDER_ACCESS` environment variable, and then invokes the L2Tests plugin method `PerformL2Tests` via `JSONRPC::LinkType`.

The L2Tests plugin registers a JSON-RPC method named `PerformL2Tests` and calls `RUN_ALL_TESTS()` to execute the compiled-in gtest suites. It also supports optional filtering via `::testing::GTEST_FLAG(filter)` when the controller provides a `test_suite_list` field in the request parameters.

### End-to-end view

```mermaid
flowchart TD
  A["Developer or CI"] --> B{"Select test level"}

  B -->|L0| L0A["appgateway_l0test (executable)"]
  L0A --> L0B["ServiceMock implements IShell + ICOMLink"]
  L0B --> L0C["Instantiate ResolverFake and ResponderFake"]
  L0A --> L0D["In-proc JSONRPC dispatcher Invoke('resolve')"]

  B -->|L2| L2A["L2testController (executable)"]
  L2A --> L2B["Start WPEFramework process (local)"]
  L2A --> L2C["Set THUNDER_ACCESS to 127.0.0.1:THUNDER_PORT"]
  L2A --> L2D["Invoke org.rdk.L2Tests.1 PerformL2Tests (JSON-RPC)"]
  L2D --> L2E["L2Tests plugin calls RUN_ALL_TESTS()"]
```

## Mocking

### L0 mocking: deterministic fakes via `ServiceMock`

In L0, mocking is implemented by providing a fake host environment rather than by gmock expectations. `L0Test::ServiceMock` controls “what the plugin sees” when it queries interfaces or tries to instantiate its dependencies.

The key fakes are:

- `L0Test::ResolverFake`, which implements `Exchange::IAppGatewayResolver` and `Exchange::IConfiguration`.
- `L0Test::ResponderFake`, which implements `Exchange::IAppGatewayResponder` and `Exchange::IConfiguration`.

`ResolverFake::Resolve(...)` implements deterministic behavior based on the requested method name:

- If the method is `"l0.notPermitted"`, it returns `Core::ERROR_PRIVILIGED_REQUEST` and a small JSON error object.
- If the method is `"l0.notSupported"`, it returns `Core::ERROR_NOT_SUPPORTED`.
- If the method is `"l0.notAvailable"`, it returns `Core::ERROR_UNAVAILABLE`.
- Otherwise, it returns `Core::ERROR_NONE` and the JSON literal `null`.

`ResponderFake` supports “transport available vs unavailable” behavior via an internal boolean and a helper method `SetTransportEnabled(bool)`. When transport is disabled, responder methods return `Core::ERROR_UNAVAILABLE`, which allows the tests to validate error-path behavior without a real WebSocket connection.

`ServiceMock::Config` allows tests to simulate missing dependencies by controlling whether the resolver and/or responder are provided. `ServiceMock::Config` also supports controlling whether the responder’s “transport” should be considered available at construction time (`responderTransportAvailable`).

### L2 mocking: disabling plugin autostart before starting Thunder

`L2testController.cpp` includes a concrete mitigation for startup ordering and dependency mocking. Before starting Thunder, it scans `./install/etc/WPEFramework/plugins/` and replaces `"autostart":true` with `"autostart":false` for all plugins except `L2Tests.json`. The comment explains why: some plugins perform IARM calls in `Initialize()`, which can crash if the relevant mocks are not ready.

This is part of the L2 orchestration “mocking story”: L2 is still meant to be testable in a controlled environment, but instead of a single in-proc fake host (L0), the environment is controlled by configuring which plugins start and when.

### Optional provider state service (Pact scaffolding)

The L2 directory also contains a provider state service implementation under:

`app-gateway2/app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/pact/providerStates/providerStateService/`

The `start.py` script starts a Flask app on port `5003`. This kind of service is typically used to establish provider states during contract testing, and it is a separate process from both Thunder and the L2 controller.

## Testing Scenarios

### L0 scenarios (examples based on current tests)

The following scenarios are directly represented by the current L0 tests and harness behavior:

- Plugin lifecycle success. `Initialize()` returns an empty string on success and `Deinitialize()` can be called cleanly afterwards.
- JSON-RPC registration and unregistration. After `Initialize()`, an `IDispatcher` can invoke `"resolve"` successfully; after `Deinitialize()`, invoking `"resolve"` should fail (typically `ERROR_UNKNOWN_METHOD`).
- Error-path coverage without external dependencies. By using `ResolverFake`, L0 tests can validate error returns like `ERROR_NOT_SUPPORTED` or `ERROR_UNAVAILABLE` deterministically.
- Malformed or missing JSON parameters. L0 tests call the dispatcher’s `Invoke(...)` and validate `ERROR_BAD_REQUEST` for invalid inputs (as described in the L0 overview documentation and supported by the l0test test suite structure).
- Missing resolver/responder dependency. If the resolver is not provided, the plugin cannot register the `"resolve"` method, and invoking it returns `ERROR_UNKNOWN_METHOD`.

### L2 scenarios (examples based on current controller + plugin implementation)

At L2, the key scenario is “start Thunder, run gtest suites, collect results”:

- Start the local Thunder process (WPEFramework).
- Set the JSON-RPC access location via `THUNDER_ACCESS` to `127.0.0.1:<THUNDER_PORT>`.
- Invoke the test plugin method `PerformL2Tests` using JSON-RPC.
- Optionally provide a filter string so only certain suites run.
- Collect results from gtest JSON output.

Because the L2Tests plugin supports `test_suite_list`, the controller can run one or more suites by building a filter string like `"SuiteA*:SuiteB*"` and sending it in the JSON-RPC parameters.

## Pros/Cons

### L0

L0 is the fastest and most deterministic test level in this repository. Because it runs entirely in-process, it is suitable for rapid iteration and for validating return codes and registration semantics without worrying about process start/stop or network access.

The main trade-off is that L0 does not test the real runtime environment. It validates AppGateway behavior against deterministic fakes, so it can miss issues that only occur when the real Thunder process is running, when plugins are activated/deactivated, or when process-level configuration and startup ordering come into play.

### L1

The shared-testframework approach (as described in the `entservices-testframework` README) reduces duplication of mocks and makes it easier to keep mock interfaces consistent across many plugins and repositories. It also aligns with typical gtest/gmock workflows, including richer assertions and expectations.

The downside of centralization is that local discovery and execution can be less straightforward. Developers often need to understand how the testframework builds and links per-repo test libraries, and how CI orchestrates the combined build.

### L2

L2 provides an integration-style execution model that is closer to how plugins are exercised in real deployments: it starts a Thunder runtime and runs tests through a Thunder plugin interface. This is particularly valuable for catching issues that depend on runtime configuration, plugin startup ordering, and JSON-RPC integration.

The trade-offs are cost and complexity. L2 runs slower than L0, relies on process orchestration, and requires careful environment isolation (for example, toggling autostart off for other plugins to avoid crashes during initialization).

## Example Code

### L0: building JSON-RPC parameters and calling `resolve`

The L0 test `AppGateway_Init_DeinitTests.cpp` builds a JSON parameter string for `resolve` and invokes it via `PluginHost::IDispatcher`:

```cpp
static std::string ResolveParamsJson(const std::string& method, const std::string& params = "{}")
{
    return std::string("{")
        + "\"requestId\": 1001,"
        + "\"connectionId\": 10,"
        + "\"appId\": \"com.example.test\","
        + "\"origin\": \"org.rdk.AppGateway\","
        + "\"method\": \"" + method + "\","
        + "\"params\": \"" + params + "\""
        + "}";
}
```

It then invokes the registered JSON-RPC method:

```cpp
auto dispatcher = ps.plugin->QueryInterface<IDispatcher>();
std::string jsonResponse;
const std::string paramsJson = ResolveParamsJson("dummy.method", "{}");
const uint32_t rc = dispatcher->Invoke(nullptr, 0, 0, "", "resolve", paramsJson, jsonResponse);
```

### L0: simulating missing resolver/responder

The L0 overview documentation includes a concrete example of disabling both resolver and responder provisioning so that `Initialize()` fails and `"resolve"` is not registered:

```cpp
PluginAndService ps{ L0Test::ServiceMock::Config(false, false) };
const std::string rc = ps.plugin->Initialize(ps.service);
// Later: dispatcher->Invoke(..., "resolve", ...) -> ERROR_UNKNOWN_METHOD
```

This behavior is implemented by `ServiceMock::Instantiate(...)` returning `nullptr` for the expected dependency instantiations.

### L2: invoking the L2Tests plugin and applying a gtest filter

The L2Tests plugin registers `"PerformL2Tests"` and supports a `test_suite_list` parameter:

```cpp
if (parameters.HasLabel("test_suite_list")) {
    const std::string& message = parameters["test_suite_list"].String();
    ::testing::GTEST_FLAG(filter) = message;
}
status = RUN_ALL_TESTS();
```

The controller builds the filter based on CLI arguments and forwards it through JSON-RPC:

```cpp
if (argc > 1) {
    message = std::string(argv[1]) + std::string("*");
    while (arguments < argc) {
        message = (message + std::string(":") + std::string(argv[arguments]) + std::string("*"));
        arguments++;
    }
    params["test_suite_list"] = message;
}
status = L2testobj->PerformL2Tests(params, result);
```

## Artifacts

### L0 artifacts

L0 tests support coverage collection using `lcov` and HTML report generation via `genhtml`. The L0 README describes a common output flow:

- `coverage.info` generated by `lcov -c ...`
- HTML report generated under a `coverage/` folder by `genhtml -o coverage coverage.info`

The L0 test harness also supports an environment variable `APPGATEWAY_RESOLUTIONS_PATH` (read via `getenv()` in the L0 tests) to point the tests at a real resolution JSON file when exercising resolution-loading behavior.

### L2 artifacts

When starting Thunder, `L2testController.cpp` exports:

- `GTEST_OUTPUT="json:$PWD/rdkL2TestResults.json"`

This means the L2 run is expected to emit a gtest JSON report named `rdkL2TestResults.json` in the current working directory of the controller process. This output is useful for CI parsing and for post-processing results outside the Thunder logs.

## References

- [AppGateway l0test README](../app-gateway/AppGateway/l0test/README.md)
- [AppGateway l0test overview](../app-gateway/AppGateway/l0test/docs/l0test-overview.md)
- [L0 ServiceMock and fakes](../app-gateway/AppGateway/l0test/ServiceMock.h)
- [L0 init/deinit test example](../app-gateway/AppGateway/l0test/AppGateway_Init_DeinitTests.cpp)
- [entservices-testframework README](../app-gateway/L1_L2_Testing/entservices-testframework/README.md)
- [L2 test controller](../app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/L2testController.cpp)
- [L2Tests plugin implementation](../app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/L2TestsPlugin/L2Tests.cpp)
- [L2Tests plugin header](../app-gateway/L1_L2_Testing/entservices-testframework/Tests/L2Tests/L2TestsPlugin/L2Tests.h)
