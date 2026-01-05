# L0 Coverage Run Blockers (latest)

## Run attempted
Command:
- `bash /home/kavia/workspace/code-generation/app-gateway2/AppGateway/l0test/run_l0_coverage.sh`

Artifacts:
- Console log captured to: `coverage/L0_COVERAGE_RUN_CONSOLE.latest.log`

## Current result
**FAIL (input checks)** — script exits before configure/build/tests.

## Primary blocker
### 1) Install prefix path mismatch in script
The script is currently using:
- `/home/kavia/workspace/code-generation/dependencies/install`

But in this repo layout, dependencies are located at:
- `/home/kavia/workspace/code-generation/app-gateway2/dependencies/install`

**Impact:** The script exits early and does not build/run tests or generate coverage output.

## Status on tooling requested by this subtask
### `lcov` / `genhtml`
Installed successfully and available on PATH:
- `/usr/bin/lcov` (LCOV 2.0-1)
- `/usr/bin/genhtml` (LCOV 2.0-1)

## Next step
Re-run the script after the install-prefix fix:
- `bash app-gateway2/AppGateway/l0test/run_l0_coverage.sh`

Expected outputs on success:
- `app-gateway2/AppGateway/l0test/build/coverage.info`
- `app-gateway2/AppGateway/l0test/coverage/index.html`
