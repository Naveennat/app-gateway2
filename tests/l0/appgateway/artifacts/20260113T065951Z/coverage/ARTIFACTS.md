# AppGateway L0 Coverage Artifacts

This directory is produced by `tests/l0/appgateway/run_coverage.sh`.

## What is included in coverage

Coverage reports are **restricted** to only:

- `app-gateway2/plugin/AppGateway/**`
- `app-gateway2/helpers/**`

Everything else is excluded from the LCOV tracefile and HTML report (SDK/vendor/framework/system headers, other plugins, etc.).

## Files

- `coverage.info`  
  LCOV tracefile used to generate the report (filtered to the paths above).

- `html/`  
  HTML report output. Open:
  - `html/index.html`
