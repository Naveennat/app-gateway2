# AppGateway2 L0 Coverage Re-run Summary (Plugin capture verification)

**Timestamped artifacts folder:** `tests/l0/appgateway/artifacts/20260113T065951Z/`

## Command
Executed from `app-gateway2/`:
- `./tests/l0/appgateway/run_coverage.sh`

## Captured outputs
- **Exit code:** `0`  
  - File: `run_coverage.exit_code.txt`
- **Stdout:** `run_coverage.stdout.log`
- **Stderr:** `run_coverage.stderr.log`

## Coverage artifacts saved
Copied from `tests/l0/appgateway/coverage/` into:
- `tests/l0/appgateway/artifacts/20260113T065951Z/coverage/`

Key files:
- `coverage/coverage.info`
- `coverage/coverage.filtered.info`
- `coverage/html/index.html` (and all HTML assets/pages)

## Verification: plugin/** appears in filtered lcov
`coverage/coverage.filtered.info` contains `SF:` entries under `app-gateway2/plugin/**`, including:

- `SF:/home/kavia/workspace/code-generation/app-gateway2/plugin/AppGateway/AppGateway.cpp`
- `SF:/home/kavia/workspace/code-generation/app-gateway2/plugin/AppGateway/AppGatewayImplementation.cpp`
- `SF:/home/kavia/workspace/code-generation/app-gateway2/plugin/AppGateway/AppGatewayResponderImplementation.cpp`
- `SF:/home/kavia/workspace/code-generation/app-gateway2/plugin/AppGateway/Resolver.cpp`
- `SF:/home/kavia/workspace/code-generation/app-gateway2/plugin/AppGateway/Module.cpp`
- (plus corresponding headers: `AppGateway.h`, `AppGatewayImplementation.h`, `AppGatewayResponderImplementation.h`, `Resolver.h`)

## Verification: plugin/** appears in HTML
The following plugin pages exist in the saved HTML report:
- `coverage/html/plugin/AppGateway/AppGateway.cpp.gcov.html`
- `coverage/html/plugin/AppGateway/AppGatewayImplementation.cpp.gcov.html`
- `coverage/html/plugin/AppGateway/Resolver.cpp.gcov.html`
- `coverage/html/plugin/AppGateway/Module.cpp.gcov.html`

Top-level HTML entrypoint:
- `coverage/html/index.html`

## Notable summary from stdout (end of run)
- Overall coverage rate:
  - **lines**: 32.8% (619 / 1887)
  - **functions**: 23.1% (64 / 277)

(Full details are available in `run_coverage.stdout.log` and `run_coverage.stderr.log`.)
