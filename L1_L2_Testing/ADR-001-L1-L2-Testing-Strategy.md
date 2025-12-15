# ADR-001: L1 and L2 Testing Strategy for RDK Services CPC

## Status
Accepted

## Context
RDK Services CPC requires a comprehensive testing strategy to ensure code quality, prevent regressions, and maintain system reliability. The project needs both unit-level testing (L1) and integration-level testing (L2) to validate individual components and their interactions with the Thunder framework.

## Decision
We have implemented a two-tier testing strategy:

### L1 Tests (Unit Tests)
- **Purpose:** Unit-level testing with complete dependency mocking
- **Framework:** GoogleTest/GoogleMock (v1.15.0)
- **Scope:** Individual component behavior validation
- **Dependencies:** All external dependencies mocked via entservices-testframework
- **Location:** `Tests/L1Tests/`

### L2 Tests (Integration Tests)
- **Purpose:** Component-level integration testing with Thunder framework
- **Framework:** GoogleTest with Thunder runtime
- **Scope:** Plugin interactions and Thunder JSONRPC communication
- **Dependencies:** Minimal mocking, actual Thunder instance running on port 9998
- **Location:** `Tests/L2Tests/`

## Implementation Details

### Test Infrastructure
```
Tests/
├── L1Tests/              # Unit tests
│   ├── tests/           # Test implementations
│   └── CMakeLists.txt   # Build configuration
├── L2Tests/              # Integration tests
│   ├── tests/           # Test implementations
│   ├── patches/         # Runtime patches for I/O operations
│   └── CMakeLists.txt   # Build configuration
└── mocks/                # Shared mock implementations
```

### CI/CD Integration
- **Workflows:** `.github/workflows/L1-tests.yml` and `.github/workflows/L2-tests.yml`
- **Triggers:** Push/PR to main, develop, sprint/**, release/**, topic/**
- **Compiler Matrix:** GCC with coverage analysis
- **Coverage Tools:** lcov, genhtml for HTML reports
- **Memory Analysis:** Valgrind for leak detection

### Build Process

#### L1 Test Build
1. Build Thunder framework (R4.4.1) and ThunderTools (R4.4.3)
2. Build entservices-cpc-apis and entservices-apis
3. Build mock library from entservices-testframework
4. Build rdkservices-cpc plugins with test flags:
   ```cmake
   -DRDK_SERVICES_L1_TEST=ON
   -DUSE_THUNDER_R4=ON
   -DPLUGIN_<NAME>=ON
   ```
5. Build entservices-testframework to create `RdkServicesL1Test` executable
6. Link test plugin libraries with GoogleTest framework

#### L2 Test Build
1. Same dependency builds as L1
2. Build rdkservices-cpc with L2 test flags:
   ```cmake
   -DRDK_SERVICE_L2_TEST=ON
   -DRDK_SERVICE_CPC_L2_TEST=ON
   -DPLUGIN_L2Tests=ON
   ```
3. Create L2Tests shared library from test files
4. Link with entservices-testframework to create `RdkServicesL2Test` executable

### Test Execution

#### L1 Execution
```bash
PATH=$INSTALL/usr/bin:${PATH}
LD_LIBRARY_PATH=$INSTALL/usr/lib:$INSTALL/usr/lib/wpeframework/plugins:${LD_LIBRARY_PATH}
GTEST_OUTPUT="json:rdkL1TestResults.json"
RdkServicesL1Test
```

#### L2 Execution
```bash
PATH=$INSTALL/usr/bin:${PATH}
LD_LIBRARY_PATH=$INSTALL/usr/lib:$INSTALL/usr/lib/wpeframework/plugins:${LD_LIBRARY_PATH}
RdkServicesL2Test
```

Both run with and without Valgrind for memory leak detection.

### Coverage Analysis
- **Tool:** lcov with gcc coverage flags
- **Compilation Flags:** `-fprofile-arcs -ftest-coverage --coverage`
- **Filters:** Excludes system headers, mocks, test code, and third-party libraries
- **Output:** HTML coverage reports via genhtml
- **Target Location:** `.lcovrc_l1` configuration for L1, `.lcovrc_l2` for L2

### Current Test Coverage

#### L1 Tests
- `test_AuthService.cpp` - AuthService plugin unit tests
- `test_LostAndFound.cpp` - LostAndFound plugin unit tests
- `test_PlaybackSync.cpp` - PlaybackSync plugin unit tests
- `test_Privacy.cpp` - Privacy plugin unit tests
- `test_UtilsFile.cpp` - File utility helper tests

#### L2 Tests
- `AuthService_L2Test.cpp` - AuthService integration tests
- `DeviceProvisioning_L2Test.cpp` - DeviceProvisioning integration tests
- `PlaybackSync_L2Test.cpp` - PlaybackSync integration tests
- `Privacy_L2Test.cpp` - Privacy integration tests

### Mock Infrastructure
Located in `Tests/mocks/` and shared with entservices-testframework:
- IARM bus mocking (`authserviceIARM.h`, `Iarm.h`)
- Security API mocking (`sec_security*.h`, `KeyProvision*.h`, `SecApiProvisioner.h`)
- Network mocking (`NetworkManagerMock.h`)
- Platform mocking (device settings, RFC, RBus, telemetry)
- Storage mocking (`StoreMock.h`)
- Third-party service mocking (`LinchpinPluginRPCMock.*`)

### Dependency Management
- **entservices-testframework:** Centralized mock repository
  - Shared across all entservices-* repos
  - Provides platform interface mocks
  - Contains common test utilities
  - Maintained separately at rdkcentral/entservices-testframework

## Consequences

### Positive
1. **Comprehensive Coverage:** Both unit and integration testing ensure thorough validation
2. **Early Detection:** L1 tests catch issues during development
3. **Integration Validation:** L2 tests validate real-world scenarios with Thunder
4. **Memory Safety:** Valgrind integration catches memory leaks early
5. **Code Quality Metrics:** Coverage reports provide visibility into test completeness
6. **CI/CD Integration:** Automated testing on every PR prevents regressions
7. **Shared Infrastructure:** entservices-testframework reduces duplication across repos

### Negative
1. **Build Complexity:** Multiple build stages increase CI time
2. **Maintenance Overhead:** Mock implementations require updates with API changes
3. **Test Environment:** L2 tests require complex directory structure setup
4. **Resource Usage:** Running both test suites with Valgrind is time-intensive
5. **Cross-Repo Dependencies:** Changes in testframework impact all dependent repos

### Neutral
1. **Test Separation:** L1 and L2 tests must be maintained separately
2. **Coverage Targets:** No specific coverage percentage mandated
3. **Local Testing:** Developers can use `act` tool to run workflows locally
4. **Patch Management:** Some L2 tests require runtime patches (e.g., `AuthService_io.patch`)

## Implementation Notes

### Local Test Execution
Developers can run tests locally using GitHub Act:
```bash
# Download act tool
curl -SL https://raw.githubusercontent.com/nektos/act/master/install.sh | bash

# Run L1 tests
./bin/act -W .github/workflows/L1-tests.yml -s GITHUB_TOKEN=<token>

# Run L2 tests
./bin/act -W .github/workflows/L2-tests.yml -s GITHUB_TOKEN=<token>
```

### Adding New Tests

#### For L1 Tests
1. Create test file in `Tests/L1Tests/tests/test_<PluginName>.cpp`
2. Add to `Tests/L1Tests/CMakeLists.txt` using `add_plugin_test_ex` macro
3. Ensure mocks are available in entservices-testframework
4. Update workflow to enable plugin build flag

#### For L2 Tests
1. Create test file in `Tests/L2Tests/tests/<PluginName>_L2Test.cpp`
2. Add conditional compilation in `Tests/L2Tests/CMakeLists.txt`:
   ```cmake
   if (PLUGIN_<NAME>)
       set(SRC_FILES ${SRC_FILES} tests/<PluginName>_L2Test.cpp)
   endif()
   ```
3. Add any required patches to `Tests/L2Tests/patches/`
4. Update workflow with plugin-specific configuration

### Test Framework Updates
When modifying mocks or test infrastructure:
1. Make changes in entservices-testframework repo
2. Update ref pointer in workflow `.yml` files to point to working branch
3. Test across all dependent repos before merging
4. Document breaking changes in testframework CHANGELOG

## References
- Thunder Framework: https://github.com/rdkcentral/Thunder
- GoogleTest Documentation: https://google.github.io/googletest/
- entservices-testframework: https://github.com/rdkcentral/entservices-testframework
- Workflow Files: `.github/workflows/L1-tests.yml`, `.github/workflows/L2-tests.yml`
- Test Documentation: `Tests/README.md`

## Version History
- **Version 1.0** (December 15, 2025): Initial ADR documenting current L1/L2 testing strategy

## Authors
- RDK Services CPC Team
- Based on entservices testing framework architecture

## Related ADRs
- (Future) ADR-002: Test Coverage Requirements
- (Future) ADR-003: Mock Management Strategy
- (Future) ADR-004: CI/CD Pipeline Architecture
