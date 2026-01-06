# Tests directory layout (app-gateway2)

This `tests/` tree contains **test harness code only** (L0/L1/L2 test sources, configs, and scripts).  
It **must not** contain AppGateway plugin implementation sources (`.cpp/.h`) copied from the plugin.

## Expected layout

- `tests/l0/appgateway/l0test/`
  - L0 test executable sources (`*.cpp`)
  - Test-only headers (e.g. `ContextUtils.h`, `ServiceMock.h`)
  - `CMakeLists.txt` for standalone building of the L0 test binary
- `tests/l0/appgateway/run_coverage.sh`
  - Builds and runs the L0 tests with coverage enabled
  - Writes coverage output to: `tests/l0/appgateway/coverage/`

## Referencing the plugin (no plugin sources in tests)

Tests reference the AppGateway plugin in two ways:

1. **Headers**: included from the plugin source tree (read-only):
   - `plugin/AppGateway/` (e.g. `#include <AppGateway.h>`)

2. **Binary artifact**: linked/loaded from the installed prefix:
   - Preferred / compatible locations under:
     - `dependencies/install/lib/wpeframework/plugins/`
     - `dependencies/install/lib/plugins/`

The installed plugin shared object is expected at (compat):
- `dependencies/install/lib/plugins/libWPEFrameworkAppGateway.so`

## Coverage output

The coverage runner writes to:
- `tests/l0/appgateway/coverage/coverage.info`
- `tests/l0/appgateway/coverage/html/index.html`
