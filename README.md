# AppGateway (imported from cm6024a498)

This repository has been pruned to contain only:

- `AppGateway/` (imported from `origin/cga-cm6024a498`)

## Build (CMake)

From the repository root:

```bash
mkdir -p build
cmake -S AppGateway -B build
cmake --build build -j
```

## L0 tests / coverage

The AppGateway l0 test sources are under `AppGateway/l0test/`.

A convenience script exists:

- `AppGateway/l0test/run_l0_coverage.sh`

Notes:
- You may need a build environment that provides Thunder/WPEFramework dependencies expected by the plugin and tests.
- See `AppGateway/l0test/README.md` for additional context.
