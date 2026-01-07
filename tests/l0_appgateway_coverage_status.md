# L0 AppGateway coverage runner status (repo checkout)

## Summary
The work item requested running:

- `./tests/l0/appgateway/run_coverage.sh`

However, this repository checkout does **not** contain that path or script:
- `tests/` only contains `ottservices_curl.sh` and `ottservices_curl_examples.md`
- No `tests/l0/` tree and no `run_coverage.sh` exist anywhere in the repo.

As a result, LCOV artifacts and filtered coverage reports for:
- `plugin/AppGateway`
- `helpers`

cannot be produced from this checkout using the requested command.

## Closest available automated checks
C++ unit tests exist under the CMake build output directory:

- `build-ottservices/tests/test_appgateway`
- `build-ottservices/tests/test_fbcommon`
- `build-ottservices/tests/test_ottpermissioncache`

CTest can run them from `build-ottservices/`, but in this environment the runtime loader may require:

- `LD_LIBRARY_PATH=/home/kavia/workspace/code-generation/app-gateway2/dependencies/install/lib`

## Known failures observed (pre-fix)
Two tests failed due to Resolver returning aliases with extra quotes (JSON-encoded string values). This was fixed by normalizing JSON string extraction in:

- `app-gateway/AppGateway/Resolver.cpp` (`ExtractStringField()`)

## Follow-up required for real coverage
To generate LCOV coverage, the repo needs one of:
1) The missing `tests/l0/appgateway/run_coverage.sh` (or equivalent), or
2) A documented CMake option/toolchain enabling `--coverage` / gcov flags plus an lcov collection step.

Once a coverage runner exists, artifacts should typically be placed under a `coverage/` folder (per the work item request), but that directory does not currently exist in this checkout.
