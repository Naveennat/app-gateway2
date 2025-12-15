# ADR-003: Mock Management Strategy

## Status
Proposed

## Context
The rdkservices-cpc project depends on numerous external systems and libraries including IARM bus, device settings, security APIs, Thunder framework, network managers, and various platform-specific components. Effective unit testing (L1) requires mocking these dependencies to achieve isolation, determinism, and fast test execution.

Currently, mocks are split between:
- **entservices-testframework:** Shared mocks for common RDK platform interfaces
- **rdkservices-cpc/Tests/mocks/:** CPC-specific mocks

This split creates challenges in maintainability, versioning, and avoiding duplication. This ADR defines a comprehensive mock management strategy.

## Decision
We will implement a centralized, versioned mock management strategy with clear ownership and contribution guidelines.

### Mock Repository Structure

#### Primary Mock Repository: entservices-testframework
**Location:** https://github.com/rdkcentral/entservices-testframework

**Contents:**
```
entservices-testframework/
├── Tests/
│   ├── mocks/
│   │   ├── platform/          # Platform HAL mocks
│   │   │   ├── devicesettings.h
│   │   │   ├── Iarm.h
│   │   │   ├── Rfc.h
│   │   │   ├── RBus.h
│   │   │   ├── Telemetry.h
│   │   │   └── Udev.h
│   │   ├── security/          # Security API mocks
│   │   │   ├── sec_security.h
│   │   │   ├── sec_security_datatype.h
│   │   │   ├── sec_security_comcastids.h
│   │   │   └── KeyProvisionClient*.h
│   │   ├── network/           # Network mocks
│   │   │   ├── NetworkManagerMock.h
│   │   │   └── wpa_ctrl_mock.h
│   │   ├── system/            # System utilities
│   │   │   ├── Wraps.h
│   │   │   ├── maintenanceMGR.h
│   │   │   └── readprocMockInterface.h
│   │   ├── storage/           # Storage mocks
│   │   │   └── StoreMock.h
│   │   └── CMakeLists.txt
│   └── headers/               # Stub headers
├── patches/                   # Thunder framework patches
└── CMakeLists.txt
```

#### Project-Specific Mocks: rdkservices-cpc/Tests/mocks/
**Contents:**
```
rdkservices-cpc/Tests/mocks/
├── authserviceIARM.h          # AuthService-specific IARM definitions
├── AuthServiceMock.h          # AuthService plugin mock
├── LinchpinPluginRPCMock.*    # Linchpin integration mocks
├── CredentialUtils.*          # CPC credential utilities
├── KeyProvision*.*            # CPC-specific key provision implementations
├── SecApiProvisioner.*        # CPC security provisioner
└── CMakeLists.txt
```

### Mock Types and Guidelines

#### Type 1: Interface Mocks (GoogleMock-based)
**Purpose:** Mock C++ interfaces and classes using GoogleMock

**Example:**
```cpp
// NetworkManagerMock.h
#pragma once
#include <gmock/gmock.h>
#include "INetworkManager.h"

namespace WPEFramework {
namespace Plugin {

class NetworkManagerMock : public Exchange::INetworkManager {
public:
    MOCK_METHOD(uint32_t, Register, (Exchange::INetworkManager::INotification*), (override));
    MOCK_METHOD(uint32_t, Unregister, (Exchange::INetworkManager::INotification*), (override));
    MOCK_METHOD(string, GetPrimaryInterface, (), (const, override));
    MOCK_METHOD(uint32_t, SetPrimaryInterface, (const string&), (override));
    // ... more methods
};

} // namespace Plugin
} // namespace WPEFramework
```

**Guidelines:**
- Use `MOCK_METHOD` macro for all virtual methods
- Match exact signature including const, override qualifiers
- Provide default actions for commonly used methods
- Document mock behavior and limitations

#### Type 2: Function Wrapper Mocks
**Purpose:** Mock C functions using function wrapping

**Example:**
```cpp
// Wraps.h - Using linker wrapping
extern "C" {
    int __real_system(const char* command);
    int __wrap_system(const char* command);
    
    FILE* __real_popen(const char* command, const char* type);
    FILE* __wrap_popen(const char* command, const char* type);
}

// Wraps.cpp - Implementation with test hooks
int __wrap_system(const char* command) {
    if (g_systemMockEnabled) {
        return g_systemMockHandler(command);
    }
    return __real_system(command);
}
```

**Compilation flags:**
```cmake
target_link_options(${MODULE_NAME} PRIVATE
    -Wl,-wrap,system
    -Wl,-wrap,popen
    -Wl,-wrap,syslog
)
```

**Guidelines:**
- Use linker wrapping (`-Wl,-wrap,<function>`) for system functions
- Provide test hook mechanism for controlling behavior
- Default to calling real function when mock disabled
- Thread-safe if mocked function used in multi-threaded context

#### Type 3: Header Replacement Mocks
**Purpose:** Replace entire headers with mock implementations

**Example:**
```cpp
// devicesettings.h - Complete mock header
#pragma once
#include <gmock/gmock.h>

namespace device {

class VideoOutputPort {
public:
    MOCK_METHOD(bool, isDisplayConnected, (), (const));
    MOCK_METHOD(std::string, getResolution, (), (const));
    MOCK_METHOD(void, setResolution, (const std::string&), ());
};

class Host {
public:
    static Host& getInstance();
    MOCK_METHOD(std::vector<VideoOutputPort>, getVideoOutputPorts, (), ());
};

} // namespace device
```

**Compiler flags:**
```cmake
target_compile_options(${MODULE_NAME} PRIVATE
    -include ${MOCK_DIR}/devicesettings.h
    -include ${MOCK_DIR}/Iarm.h
)
```

**Guidelines:**
- Recreate header API surface with mocks
- Use `-include` to force inclusion before actual headers
- Document any API differences from real headers
- Maintain API compatibility across mock updates

#### Type 4: Stub Implementations
**Purpose:** Minimal implementations for headers-only dependencies

**Example:**
```cpp
// sec_security_datatype.h - Data structure stubs
#pragma once

typedef struct {
    unsigned char data[32];
    size_t length;
} SecApiKey;

typedef enum {
    SEC_API_SUCCESS = 0,
    SEC_API_FAILURE = -1
} SecApiResult;

// Minimal definitions to satisfy compilation
```

**Guidelines:**
- Provide only what's needed for compilation
- No behavior implementation
- Use in conjunction with function mocks for behavior

### Mock Lifecycle Management

#### Creating New Mocks

**Step 1: Determine Mock Location**
```
Decision Tree:
├─ Is dependency used across multiple entservices repos?
│  ├─ YES → Add to entservices-testframework
│  └─ NO ─┐
│         ├─ Is dependency CPC-specific?
│         │  ├─ YES → Add to rdkservices-cpc/Tests/mocks/
│         │  └─ NO → Consider if it should be shared (future repos may need it)
```

**Step 2: Choose Mock Type**
- C++ interfaces → Interface Mock (GoogleMock)
- C functions → Function Wrapper Mock
- Complex headers → Header Replacement Mock
- Data structures only → Stub Implementation

**Step 3: Implement Mock**
```cpp
// Template for GoogleMock interface
#pragma once
#include <gmock/gmock.h>
#include "OriginalInterface.h"

class MockClassName : public OriginalInterface {
public:
    MockClassName();
    virtual ~MockClassName();
    
    // Mock all virtual methods
    MOCK_METHOD(ReturnType, MethodName, (Args...), (override));
    
    // Provide default behaviors
    void SetupDefaults();
    
private:
    // Test helper members
};
```

**Step 4: Write Mock Tests**
```cpp
// Verify mock behaves correctly
TEST(MockClassNameTest, DefaultBehavior) {
    MockClassName mock;
    mock.SetupDefaults();
    
    EXPECT_CALL(mock, MethodName(_))
        .Times(AtLeast(1))
        .WillRepeatedly(Return(expected));
        
    // Test default behavior
}
```

**Step 5: Document Mock**
```cpp
/**
 * @brief Mock implementation of OriginalInterface
 * 
 * @details This mock provides:
 * - Full API surface mocking
 * - Default behavior via SetupDefaults()
 * - Thread-safe operation
 * 
 * Limitations:
 * - Does not mock MethodX (not needed for current tests)
 * - Synchronous only (no async callback support)
 * 
 * @example
 * MockClassName mock;
 * EXPECT_CALL(mock, MethodName(_)).WillOnce(Return(value));
 */
```

#### Updating Existing Mocks

**Version Compatibility:**
- Mock updates must maintain backward compatibility within major version
- Breaking changes require version bump and migration guide
- Deprecation warnings before removal (minimum 3 months)

**Update Process:**
1. Create feature branch in testframework repo
2. Implement mock changes with version notes
3. Test against all dependent repos (rdkservices-cpc, others)
4. Submit PR with impact analysis
5. Get approval from all affected repo maintainers
6. Merge and tag new version
7. Update dependent repos to new version

**Breaking Change Example:**
```cpp
// v1.0 - Original
MOCK_METHOD(string, GetValue, (), (const));

// v2.0 - Breaking change (return type changed)
MOCK_METHOD(JsonObject, GetValue, (), (const));

// Migration required in dependent repos
```

#### Deprecating Mocks

When a mock is no longer needed:
1. Mark as deprecated with `[[deprecated("reason")]]`
2. Document replacement in comment
3. Keep deprecated mock for 2 releases
4. Remove after all dependents migrated

```cpp
[[deprecated("Use NewMockClass instead. Removed in v3.0")]]
class OldMockClass { ... };
```

### Mock Build Integration

#### CMake Organization

**entservices-testframework/Tests/mocks/CMakeLists.txt:**
```cmake
# Build TestMock library
add_library(TestMock SHARED
    # Platform mocks
    platform/devicesettings.cpp
    platform/Iarm.cpp
    platform/Rfc.cpp
    
    # Security mocks
    security/sec_security.cpp
    security/KeyProvisionClient.cpp
    
    # Network mocks
    network/NetworkManager.cpp
    
    # System mocks
    system/Wraps.cpp
)

target_link_libraries(TestMock
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    gmock
    gtest
)

install(TARGETS TestMock DESTINATION lib)
```

**rdkservices-cpc/Tests/mocks/CMakeLists.txt:**
```cmake
# Build CPC-specific mocks
add_library(CPCTestMock SHARED
    AuthServiceMock.cpp
    LinchpinPluginRPCMock.cpp
    CredentialUtils.cpp
    # ... other CPC-specific mocks
)

target_link_libraries(CPCTestMock
    TestMock  # Depend on shared mocks
    ${NAMESPACE}Plugins::${NAMESPACE}Plugins
    gmock
)

install(TARGETS CPCTestMock DESTINATION lib)
```

#### Compilation Flags

**L1 Test Compilation with Mocks:**
```cmake
target_compile_options(${MODULE_NAME} PRIVATE
    # Include mock directories
    -I${TESTFRAMEWORK_DIR}/Tests/mocks
    -I${TESTFRAMEWORK_DIR}/Tests/headers
    
    # Force-include mock headers
    -include ${TESTFRAMEWORK_DIR}/Tests/mocks/devicesettings.h
    -include ${TESTFRAMEWORK_DIR}/Tests/mocks/Iarm.h
    -include ${TESTFRAMEWORK_DIR}/Tests/mocks/Rfc.h
    -include ${PROJECT_SOURCE_DIR}/Tests/mocks/authserviceIARM.h
)

target_link_options(${MODULE_NAME} PRIVATE
    # Wrap system functions
    -Wl,-wrap,system
    -Wl,-wrap,popen
    -Wl,-wrap,syslog
)
```

### Mock Best Practices

#### 1. Single Responsibility
Each mock should mock one interface or module:
```cpp
// Good - Focused mock
class IARMBusMock { ... };

// Bad - Too broad
class PlatformMock { /* IARM + Device Settings + RFC */ };
```

#### 2. Reasonable Defaults
Provide sensible default behaviors:
```cpp
class NetworkManagerMock {
public:
    void SetupDefaults() {
        ON_CALL(*this, GetPrimaryInterface())
            .WillByDefault(Return("eth0"));
        ON_CALL(*this, IsConnected())
            .WillByDefault(Return(true));
    }
};
```

#### 3. Test Helpers
Include helper methods for common test scenarios:
```cpp
class AuthServiceMock {
public:
    void SimulateSuccessfulAuth() {
        EXPECT_CALL(*this, authenticate(_))
            .WillOnce(Return(AuthResult::SUCCESS));
    }
    
    void SimulateAuthFailure(AuthError error) {
        EXPECT_CALL(*this, authenticate(_))
            .WillOnce(Return(error));
    }
};
```

#### 4. Clear Documentation
Document mock limitations and assumptions:
```cpp
/**
 * @brief Mock for IARM bus communication
 * 
 * @note This mock is synchronous. Real IARM bus is asynchronous.
 * @note Does not simulate bus failures or timeouts.
 * @note Thread-safe for concurrent calls.
 */
class IARMBusMock { ... };
```

#### 5. Verification Helpers
Provide verification methods:
```cpp
class IARMBusMock {
public:
    bool WasEventSent(const string& eventName) const {
        return sentEvents.count(eventName) > 0;
    }
    
    int GetCallCount(const string& method) const {
        return callCounts.at(method);
    }
    
private:
    std::set<string> sentEvents;
    std::map<string, int> callCounts;
};
```

### Mock Testing

#### Mock Validation Tests
Mocks themselves should be tested:

```cpp
// Tests/mocks/test/test_IARMBusMock.cpp
TEST(IARMBusMockTest, RegisterEvent_TracksRegistration) {
    IARMBusMock mock;
    
    EXPECT_CALL(mock, IARM_Bus_RegisterEventHandler(_, _, _))
        .Times(1)
        .WillOnce(Return(IARM_RESULT_SUCCESS));
    
    auto result = mock.IARM_Bus_RegisterEventHandler(
        "owner", IARM_BUS_EVENT, nullptr);
    
    EXPECT_EQ(result, IARM_RESULT_SUCCESS);
    EXPECT_TRUE(mock.IsEventRegistered("owner", IARM_BUS_EVENT));
}
```

### Versioning and Releases

#### Version Scheme
entservices-testframework follows semantic versioning:
- **MAJOR:** Breaking changes to mock APIs
- **MINOR:** New mocks or backward-compatible additions
- **PATCH:** Bug fixes in existing mocks

**Example:** v2.3.1
- v2 = Major version (breaking changes from v1)
- 3 = Added 3 new mock features
- 1 = First bug fix release

#### Release Process
1. Update CHANGELOG.md with all changes
2. Tag release in Git: `git tag v2.3.1`
3. Notify dependent repos of new version
4. Update rdkservices-cpc workflow files with new ref:
   ```yaml
   - name: Checkout entservices-testframework
     uses: actions/checkout@v3
     with:
       repository: rdkcentral/entservices-testframework
       ref: v2.3.1  # Updated version
   ```

### Cross-Repository Coordination

#### Workflow Branch Synchronization
When developing features requiring both code and mock changes:

**rdkservices-cpc/.github/workflows/L1-tests.yml:**
```yaml
- name: Checkout entservices-testframework
  uses: actions/checkout@v3
  with:
    repository: rdkcentral/entservices-testframework
    # Use feature branch during development
    ref: ${{ github.event.pull_request.head.ref || 'develop' }}
    # After merge, switch to version tag
    # ref: v2.3.1
```

#### Mock Change Coordination Process
1. **Discovery:** Identify need for mock changes
2. **Design:** Document required mock API changes
3. **Testframework PR:** Create PR in testframework repo
4. **Dependent PR:** Create PR in rdkservices-cpc pointing to testframework branch
5. **Parallel Review:** Review both PRs together
6. **Merge Order:** Merge testframework first, then rdkservices-cpc
7. **Version Update:** Update rdkservices-cpc to use tagged testframework version

## Consequences

### Positive
1. **Centralization:** Shared mocks reduce duplication across repos
2. **Consistency:** Common mock behavior across all entservices projects
3. **Maintainability:** Single source of truth for platform mocks
4. **Versioning:** Clear version management and compatibility
5. **Reusability:** New projects benefit from existing mock infrastructure
6. **Isolation:** True unit tests with complete dependency mocking
7. **Determinism:** Predictable test behavior without real systems

### Negative
1. **Coordination Overhead:** Cross-repo changes require synchronization
2. **Version Conflicts:** Different repos may need different mock versions
3. **Mock Drift:** Mocks may diverge from real implementations
4. **Learning Curve:** Developers must understand mock architecture
5. **Build Complexity:** Additional dependencies and build steps

### Neutral
1. **Repository Split:** Requires managing multiple repositories
2. **Documentation Burden:** Mocks must be well-documented
3. **Review Process:** Mock changes need cross-team review

## Alternatives Considered

### Alternative 1: All Mocks in Each Repo
- **Pros:** Complete autonomy, no cross-repo dependencies
- **Cons:** Massive duplication, inconsistent behavior, maintenance nightmare
- **Rejected:** Violates DRY principle, unmaintainable at scale

### Alternative 2: Mocks as Separate Packages
- **Pros:** Version management, dependency resolution
- **Cons:** Additional infrastructure, publishing overhead
- **Rejected:** Overkill for current scale, adds complexity

### Alternative 3: Git Submodules
- **Pros:** Built-in Git feature, pinned versions
- **Cons:** Difficult to use, error-prone, unpopular with developers
- **Rejected:** Poor developer experience

### Alternative 4: No Mocking (Integration Tests Only)
- **Pros:** Tests real behavior, no mock maintenance
- **Cons:** Slow, brittle, requires full environment setup
- **Rejected:** Not suitable for unit testing

## Implementation Checklist

- [ ] Audit existing mocks in rdkservices-cpc/Tests/mocks/
- [ ] Identify candidates for migration to testframework
- [ ] Create migration plan with backward compatibility
- [ ] Document mock API standards and guidelines
- [ ] Establish mock versioning and release process
- [ ] Set up mock validation tests
- [ ] Train team on mock development and usage
- [ ] Create mock contribution guidelines
- [ ] Implement automated mock testing in CI/CD

## References
- GoogleMock Documentation: https://google.github.io/googletest/gmock_cook_book.html
- entservices-testframework: https://github.com/rdkcentral/entservices-testframework
- Semantic Versioning: https://semver.org/
- Test Doubles (Fowler): https://martinfowler.com/bliki/TestDouble.html

## Revision History
- **Version 1.0** (December 15, 2025): Initial mock management strategy proposal

## Related ADRs
- ADR-001: L1 and L2 Testing Strategy
- ADR-002: Test Coverage Requirements
- ADR-004: CI/CD Pipeline Architecture
