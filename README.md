# app-gateway2 (AppGateway plugin + tests)

This workspace contains:

- `plugin/AppGateway/`  
  **IMMUTABLE / upstream-synced** copy of `rdkcentral/entservices-infra` (branch `develop`) `AppGateway/`.  
  Do **not** edit any sources in this directory. See: `plugin/AppGateway/IMMUTABLE_UPSTREAM.md`.

- `tests/l0/appgateway/`  
  All test-only code, harnesses, coverage scripts, and test CMake live here.

- `interfaces/`  
  Local interface headers and lightweight JSON stubs needed for building/linking in CI.

## Build the AppGateway plugin (no source modification)

From this workspace root:

```bash
./build_plugin_appgateway.sh
```

This configures/builds/installs the plugin into:

- `dependencies/install`

## L0 tests / coverage

See:

- `tests/l0/appgateway/l0test/README.md` (if present)
- `tests/l0/appgateway/l0test/CMakeLists.txt`

All test-related changes must remain under `tests/l0/appgateway/**`.
