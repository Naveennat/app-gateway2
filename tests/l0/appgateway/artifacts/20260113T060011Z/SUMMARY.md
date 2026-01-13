# L0 Coverage Run Summary (app-gateway2)

- **Timestamp (UTC):** 20260113T060011Z
- **Command:** `./tests/l0/appgateway/run_coverage.sh`
- **Working dir:** `app-gateway2/`
- **Exit code:** `0` (SUCCESS)

## Logs captured
- **stdout:** `run_coverage.stdout.log`
- **stderr:** `run_coverage.stderr.log`
- **exit code file:** `run_coverage.exit_code.txt`

## Coverage artifacts copied into this run folder
Copied from `tests/l0/appgateway/coverage/` to:
- `coverage/coverage.info`
- `coverage/coverage.filtered.info`
- `coverage/html/` (HTML report)
- **Copied artifacts size:** ~640K

HTML report entrypoint:
- `tests/l0/appgateway/artifacts/20260113T060011Z/coverage/html/index.html`

## Coverage summary (from stdout)
Overall coverage rate:
- lines......: **85.7% (18 of 21 lines)**
- functions..: **83.3% (5 of 6 functions)**
- branches...: no data found

Files listed in report:
- `plugin/AppGateway/AppGatewayImplementation.h`
- `plugin/AppGateway/AppGateway.h`
- `helpers/rdkservices-comcast/helpers/StringUtils.h`

## Notes
- No failing compiler errors detected (run succeeded).
- stderr contains `geninfo`/`lcov` warnings about JSON::PP performance and unused include patterns; these did not fail the run.

## Verification
- A follow-up verification run of `./tests/l0/appgateway/run_coverage.sh` was executed and also succeeded (exit code `0`).
- Verification logs were captured under `tests/l0/appgateway/artifacts/` as:
  - `coverage_verify_20260113T*.stdout.log`
  - `coverage_verify_20260113T*.stderr.log`
