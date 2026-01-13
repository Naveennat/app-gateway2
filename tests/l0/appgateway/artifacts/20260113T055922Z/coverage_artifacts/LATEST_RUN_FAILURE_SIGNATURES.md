# AppGateway L0 Coverage – Latest Failure Signatures

This file is generated as part of the harness evaluation to summarize the latest observed failure signatures and where to find artifacts.

## Command executed
- `tests/l0/appgateway/run_coverage.sh` (as wired)

## Artifacts
- Latest harness console: `tests/l0/appgateway/coverage/console.latest.txt`
- Latest combined result: `tests/l0/appgateway/coverage/RESULT.latest.txt`
- Additional captured console (alternate run): `/tmp/appgateway_l0_coverage_console.txt`

## Primary runtime failure signature (latest)
Repeated across tests:
- `Missing implementation classname AppGatewayImplementation in library`
- `Initialize: Failed to initialise AppGatewayResolver plugin!`
- Follow-on failures:
  - `Could not retrieve the AppGateway interface.`
  - Many asserts expect `0` (ERROR_NONE) but see `53` (handler missing/unavailable)

## Process abort (latest)
- `double free or corruption (out)`
- `Aborted (core dumped)`

## Alternate/earlier build-stop signature (captured)
- `fatal error: core/core.h: No such file or directory`
(Occurs when the configured PREFIX/include layout does not provide `<core/core.h>`.)

## Notes
No plugin source code was modified to produce these signatures.
