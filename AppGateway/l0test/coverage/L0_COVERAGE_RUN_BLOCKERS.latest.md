# L0 Coverage Run Blockers (latest)

## Run attempted
Command:
- `bash /home/kavia/workspace/code-generation/app-gateway2/AppGateway/l0test/run_l0_coverage.sh`

Artifacts:
- Console log captured to: `coverage/L0_COVERAGE_RUN_CONSOLE.latest.log`

## Current result
**FAIL (pre-flight tool check)** — script exits before configure/build/tests.

## Primary blocker
### 1) `lcov` not available in PATH
The script requires `lcov` and `genhtml` (both provided by the `lcov` package). Current run output:

- `[run_l0_coverage][ERROR] lcov not found in PATH. Install hint: sudo apt install -y lcov`

**Impact:** No build, no test execution, and no coverage info/html report can be generated.

## Secondary / downstream blocker (previously observed)
Once `lcov` is installed and the script proceeds to build/link, prior runs indicated a likely **AppGateway interface/API mismatch** leading to link errors for `appgateway_l0test` (Thunder/WPEFramework R4_4 / 4.4).

See older evidence in:
- `coverage/L0_COVERAGE_RUN_LOGS.latest.txt`
- `coverage/L0_COVERAGE_BUILD_AND_RUN.log`

## Suggested next step
Install `lcov` (provides `lcov` + `genhtml`) in the execution environment, then re-run:
- `bash app-gateway2/AppGateway/l0test/run_l0_coverage.sh`

If the build then fails at link again, address the interface mismatch as indicated by the link errors.
