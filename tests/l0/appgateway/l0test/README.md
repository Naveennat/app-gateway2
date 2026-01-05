# AppGateway l0test (offline/deterministic)

This directory contains a minimal level-0 test binary for the AppGateway plugin.
It uses an in-proc `ServiceMock` that implements `PluginHost::IShell` and `IShell::ICOMLink`,
allowing `IShell::Root<T>()` to return deterministic fakes instead of spawning processes.

## Build & Run (from repo root)

Configure:
```sh
cmake -G Ninja -S app-gateway2/app-gateway -B build/appgatewayl0test -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=$PWD/dependencies/install -DCMAKE_INSTALL_PREFIX=$PWD/dependencies/install
```

Build:
```sh
cmake --build build/appgatewayl0test --target appgateway_l0test -j
```

Run:
```sh
export LD_LIBRARY_PATH=$PWD/build/appgatewayl0test/AppGateway:$PWD/dependencies/install/lib:$LD_LIBRARY_PATH
./build/appgatewayl0test/AppGateway/appgateway_l0test
```

## Coverage (lcov)

If needed:
```sh
sudo apt install -y lcov
```

Repo-root style output (as requested):
```sh
lcov -c -o coverage.info -d build/appgatewayl0test
genhtml -o coverage coverage.info
```

Or use the helper script:
```sh
bash app-gateway2/app-gateway/scripts/appgateway_l0_coverage.sh build/appgatewayl0test
```

Or from the build directory via CMake target:
```sh
cmake --build build/appgatewayl0test --target appgateway_l0test_coverage -j
# outputs: build/appgatewayl0test/coverage/index.html
```
