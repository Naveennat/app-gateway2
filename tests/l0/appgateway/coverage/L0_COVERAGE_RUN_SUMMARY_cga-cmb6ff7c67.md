# AppGateway L0 Coverage Run Summary — cga-cmb6ff7c67

## Git context
- Repo: `app-gateway2/`
- Branch: `cga-cmb6ff7c67`
- Commit: `0155d6a0e8653f3a518d0d3e621f9389652115d0`

## Workflow executed
- Script: `app-gateway2/tests/l0/appgateway/run_coverage.sh`

## Exit status
- **FAIL** (script terminates early during plugin build)
- Primary failure: missing headers during AppGateway plugin build
  - `UtilsLogging.h` not found
  - `WsManager.h` not found

Evidence (from generated error snippet):
- `app-gateway2/tests/l0/appgateway/coverage/build_plugin_appgateway_errors.latest.txt`

## Coverage results (lcov summary)
Because the plugin build fails before tests run, the generated LCOV tracefile is effectively a minimal/placeholder trace and reports:

- Lines: **0.0%** (0 of 2)
- Functions: **0.0%** (0 of 2)

Command used to extract (tolerating partial/inconsistent tracefile):
- `lcov --summary tests/l0/appgateway/coverage/coverage.info --ignore-errors inconsistent,corrupt,empty`

## Coverage artifacts (paths)
Despite the failure, the workflow produced these artifacts:

- LCOV tracefile:
  - `app-gateway2/tests/l0/appgateway/coverage/coverage.info`

- HTML report:
  - `app-gateway2/tests/l0/appgateway/coverage/html/index.html`

- Console/run log captured during execution:
  - `app-gateway2/tests/l0/appgateway/coverage/latest_run.log`

## Notes
- The failure occurs during the **“Build AppGateway plugin (ensure up-to-date)”** stage inside `run_coverage.sh`, before L0 tests execute.
- The LCOV/HTML output appears to be generated from minimal coverage records (not from a successful test run).
