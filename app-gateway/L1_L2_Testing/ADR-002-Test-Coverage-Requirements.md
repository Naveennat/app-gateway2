# ADR-002: Test Coverage Requirements

## Status
Proposed

## Context
The rdkservices-cpc project implements critical RDK services including AuthService, Privacy, DeviceProvisioning, PlaybackSync, and LostAndFound. These services handle sensitive operations such as authentication, device provisioning, and privacy controls. To ensure reliability and prevent regressions, we need clear test coverage requirements and quality metrics.

Currently, the project generates code coverage reports using lcov/genhtml during CI/CD execution, but there are no formal coverage targets or enforcement mechanisms. This ADR establishes coverage requirements and quality gates.

## Decision
We will implement the following test coverage requirements:

### Coverage Targets

#### Overall Project Coverage
- **Minimum Line Coverage:** 80%
- **Minimum Function Coverage:** 85%
- **Minimum Branch Coverage:** 75%
- **Target Line Coverage:** 90%

#### Per-Plugin Coverage Requirements
| Plugin | Minimum Line Coverage | Minimum Branch Coverage | Rationale |
|--------|----------------------|------------------------|-----------|
| AuthService | 85% | 80% | Security-critical authentication |
| Privacy | 85% | 80% | Privacy-sensitive operations |
| DeviceProvisioning | 85% | 80% | Device security and provisioning |
| PlaybackSync | 80% | 75% | Playback coordination logic |
| LostAndFound | 80% | 75% | Device management features |
| Helpers/Utilities | 90% | 85% | Shared code, high reuse |

#### Coverage Exclusions
The following code is exempt from coverage requirements:
- Auto-generated code (Thunder plugin boilerplate)
- Third-party library integration stubs
- Platform-specific conditional compilation blocks marked with `// LCOV_EXCL_START` and `// LCOV_EXCL_STOP`
- Debug-only code paths (e.g., `#ifdef DEBUG_LOGGING`)
- Unreachable error handlers for catastrophic failures

### Coverage Measurement

#### Tools and Configuration
- **Tool:** lcov (Linux Test Project coverage tool)
- **Configuration Files:**
  - L1: `Tests/L1Tests/.lcovrc_l1`
  - L2: `Tests/L2Tests/.lcovrc_l2`
- **Compiler Flags:** `-fprofile-arcs -ftest-coverage --coverage`
- **Report Format:** HTML via genhtml, JSON for automation

#### Coverage Collection
```bash
# L1 Coverage
lcov -c -o coverage.info -d build/rdkservices-cpc -d build/entservices-testframework

# Apply filters
lcov -r coverage.info \
  '/usr/include/*' \
  '*/build/rdkservices-cpc/_deps/*' \
  '*/build/entservices-testframework/_deps/*' \
  '*/install/usr/include/*' \
  '*/Tests/headers/*' \
  '*/Tests/mocks/*' \
  '*/Tests/L1Tests/*' \
  '*/googlemock/*' \
  '*/googletest/*' \
  -o filtered_coverage.info

# Generate HTML report
genhtml -o coverage -t "rdkservices-cpc coverage" filtered_coverage.info
```

### Coverage Enforcement

#### CI/CD Integration
1. **Coverage Reports Generated:** Every PR and commit to main/develop branches
2. **Coverage Artifacts:** Published as GitHub Actions artifacts (30-day retention)
3. **Coverage Trends:** Track coverage changes over time
4. **Quality Gates:** PR cannot merge if coverage drops below minimum thresholds

#### Pull Request Requirements
- **Coverage Delta:** New code must maintain or improve overall coverage
- **No Regression:** Cannot decrease coverage by more than 0.5% without justification
- **New Features:** New plugins/features must include tests achieving minimum targets
- **Bug Fixes:** Bug fixes should include regression tests

#### Coverage Review Process
1. Developer runs tests locally before submitting PR
2. CI/CD generates coverage report on PR
3. Automated check validates coverage meets minimums
4. Code reviewer examines coverage report for untested critical paths
5. Team lead approves coverage exceptions with documented rationale

### Test Quality Requirements

Beyond coverage percentages, tests must meet quality standards:

#### Unit Test (L1) Requirements
- **Isolation:** Each test must be independent and idempotent
- **Mocking:** All external dependencies mocked appropriately
- **Assertions:** Clear, specific assertions with meaningful failure messages
- **Edge Cases:** Test boundary conditions, error paths, and edge cases
- **Performance:** Individual test execution < 100ms
- **Naming:** Descriptive test names following pattern: `Test<Component>_<Scenario>_<ExpectedBehavior>`

Example:
```cpp
TEST_F(AuthServiceTest, GetToken_WithValidCredentials_ReturnsValidToken)
{
    // Arrange
    EXPECT_CALL(*mockCredentialStore, getCredentials())
        .WillOnce(Return(validCredentials));
    
    // Act
    JsonObject response;
    authService->getToken(request, response);
    
    // Assert
    ASSERT_TRUE(response.HasLabel("token"));
    EXPECT_THAT(response["token"].String(), Not(IsEmpty()));
    EXPECT_EQ(response["success"].Boolean(), true);
}
```

#### Integration Test (L2) Requirements
- **Real Interactions:** Test actual plugin behavior with Thunder framework
- **State Management:** Properly initialize and cleanup Thunder context
- **Async Handling:** Correctly handle asynchronous operations and notifications
- **Timeouts:** Use appropriate timeouts for async operations (default: 5s)
- **Error Scenarios:** Test plugin behavior under error conditions
- **JSONRPC Validation:** Verify correct JSONRPC request/response formats

Example:
```cpp
TEST_F(AuthServiceL2Test, Activate_ThenGetToken_ReturnsSuccess)
{
    // Activate plugin
    ASSERT_EQ(plugin->Activate(), Core::ERROR_NONE);
    WaitForPluginState("activated", 5000);
    
    // Call method
    JsonObject params;
    params["type"] = "device";
    JsonObject result;
    
    ASSERT_EQ(InvokeServiceMethod("getToken", params, result), Core::ERROR_NONE);
    EXPECT_TRUE(result["success"].Boolean());
    EXPECT_FALSE(result["token"].String().empty());
}
```

### Coverage Reporting

#### Report Contents
Coverage reports must include:
- **Summary Statistics:** Overall line, function, and branch coverage percentages
- **Per-File Breakdown:** Coverage for each source file
- **Per-Plugin Summary:** Aggregated coverage by plugin
- **Trend Analysis:** Coverage change compared to previous build
- **Critical Gaps:** Highlighted uncovered critical code paths

#### Report Distribution
- **HTML Reports:** Published as GitHub Actions artifacts
- **PR Comments:** Bot posts coverage summary on pull requests
- **Dashboard:** Coverage trends displayed on project dashboard
- **Email Notifications:** Weekly coverage reports to team leads

### Exception Process

#### Requesting Coverage Exemptions
When minimum coverage cannot be achieved:

1. **Document Reason:** File exemption request with technical justification
2. **Alternative Validation:** Describe alternative testing/validation approach
3. **Risk Assessment:** Identify and document risks of reduced coverage
4. **Approval Required:** Technical lead and QA lead must approve
5. **Time-Bound:** Exemptions expire after 3 months and must be renewed
6. **Tracking:** Log all exemptions in `docs/coverage-exemptions.md`

#### Valid Exemption Reasons
- Platform-specific code that cannot be tested in CI environment
- Hardware-dependent functionality requiring physical device
- Third-party API integration with mock limitations
- Legacy code scheduled for deprecation
- Race conditions or timing-sensitive code difficult to test deterministically

### Metrics and Monitoring

#### Key Performance Indicators (KPIs)
- **Coverage Trend:** Monthly coverage percentage over time
- **Test Execution Time:** Total time for L1 and L2 test suites
- **Test Stability:** Flaky test rate (should be < 1%)
- **Code Churn vs Coverage:** Correlation between code changes and coverage
- **Bug Escape Rate:** Production bugs that lacked test coverage

#### Quarterly Reviews
- Review overall coverage trends
- Identify plugins below target coverage
- Assess test quality and effectiveness
- Adjust coverage targets based on project maturity

## Consequences

### Positive
1. **Quality Assurance:** Minimum coverage ensures baseline testing rigor
2. **Regression Prevention:** Coverage requirements catch undertested changes
3. **Code Confidence:** Developers and stakeholders trust well-tested code
4. **Maintainability:** High coverage facilitates refactoring
5. **Documentation:** Tests serve as executable specifications
6. **Early Detection:** Issues found in development, not production
7. **Compliance:** Meets industry standards for safety-critical systems

### Negative
1. **Development Time:** Achieving high coverage requires time investment
2. **False Security:** High coverage doesn't guarantee bug-free code
3. **Test Maintenance:** Large test suites require ongoing maintenance
4. **Pressure to Game Metrics:** Developers might write tests just to hit numbers
5. **CI/CD Duration:** Coverage analysis adds time to pipeline

### Neutral
1. **Learning Curve:** New developers need training on coverage tools
2. **Tooling Costs:** Maintaining lcov and coverage infrastructure
3. **Storage Requirements:** Coverage artifacts consume storage space

## Implementation Plan

### Phase 1: Baseline Assessment (Week 1-2)
- [ ] Run full coverage analysis on current codebase
- [ ] Document current coverage by plugin
- [ ] Identify gaps preventing minimum coverage
- [ ] Establish coverage tracking dashboard

### Phase 2: Tooling Setup (Week 3-4)
- [ ] Enhance CI/CD workflows with coverage gates
- [ ] Implement automated coverage reporting bot
- [ ] Create coverage exemption tracking system
- [ ] Set up coverage trend monitoring

### Phase 3: Gap Closure (Week 5-12)
- [ ] Prioritize critical uncovered code paths
- [ ] Write tests to achieve minimum coverage
- [ ] Review and document valid exemptions
- [ ] Train team on coverage requirements

### Phase 4: Enforcement (Week 13+)
- [ ] Enable coverage gates on PR merges
- [ ] Establish weekly coverage review meetings
- [ ] Monitor compliance and address violations
- [ ] Continuously improve test quality

## Alternatives Considered

### Alternative 1: No Formal Coverage Requirements
- **Pros:** Less overhead, faster development
- **Cons:** Inconsistent quality, higher defect rates
- **Rejected:** Too risky for production services

### Alternative 2: 100% Coverage Requirement
- **Pros:** Maximum confidence, exhaustive testing
- **Cons:** Unrealistic, diminishing returns, test bloat
- **Rejected:** Not practical for real-world projects

### Alternative 3: Plugin-Specific Requirements Only
- **Pros:** Flexibility for different plugin complexities
- **Cons:** Harder to enforce, inconsistent standards
- **Rejected:** Prefer consistent baseline with exemptions

### Alternative 4: Manual Coverage Reviews
- **Pros:** Human judgment, context-aware
- **Cons:** Subjective, not scalable, inconsistent
- **Rejected:** Need automated enforcement with manual oversight

## References
- lcov Documentation: http://ltp.sourceforge.net/coverage/lcov.php
- Google Test Coverage Best Practices: https://google.github.io/googletest/
- MISRA C++ Guidelines on Test Coverage
- ISO 26262 Software Testing Requirements
- `.github/workflows/L1-tests.yml` - Current L1 coverage implementation
- `.github/workflows/L2-tests.yml` - Current L2 coverage implementation

## Revision History
- **Version 1.0** (December 15, 2025): Initial coverage requirements proposal

## Related ADRs
- ADR-001: L1 and L2 Testing Strategy
- ADR-003: Mock Management Strategy
- ADR-004: CI/CD Pipeline Architecture
