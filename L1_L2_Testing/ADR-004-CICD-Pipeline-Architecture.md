# ADR-004: CI/CD Pipeline Architecture

## Status
Proposed

## Context
The rdkservices-cpc project requires a robust CI/CD pipeline to ensure code quality, prevent regressions, and automate testing, coverage analysis, and deployment. The current implementation uses GitHub Actions with workflows for L1 tests, L2 tests, Coverity scanning, and JSON validation.

As the project scales with more plugins, developers, and integration points, we need a well-architected CI/CD pipeline that balances thoroughness, speed, and resource efficiency. This ADR defines the comprehensive CI/CD architecture.

## Decision
We will implement a multi-stage CI/CD pipeline with parallel execution, smart caching, and quality gates.

### Pipeline Architecture Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        TRIGGER EVENTS                            │
│  • Push to: main, develop, sprint/**, release/**, topic/**      │
│  • Pull Request to: main, develop, sprint/**, release/**        │
│  • Manual dispatch for component releases                       │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    STAGE 1: VALIDATION (Parallel)                │
├─────────────────────┬───────────────────┬───────────────────────┤
│  JSON Validation    │  Code Format      │  Static Analysis      │
│  • Plugin configs   │  • clang-format   │  • cppcheck           │
│  • API definitions  │  • Linting        │  • Compiler warnings  │
│  Duration: ~2 min   │  Duration: ~1 min │  Duration: ~3 min     │
└─────────────────────┴───────────────────┴───────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    STAGE 2: BUILD (Parallel)                     │
├──────────────────────────────┬──────────────────────────────────┤
│     L1 Test Build            │       L2 Test Build              │
│  • Thunder Framework         │  • Thunder Framework             │
│  • ThunderTools              │  • ThunderTools                  │
│  • entservices-cpc-apis      │  • entservices-cpc-apis          │
│  • Mock Library              │  • Mock Library                  │
│  • Plugin Libraries (Debug)  │  • Plugin Libraries (Debug)      │
│  • Test Executable           │  • Test Executable               │
│  Duration: ~15 min           │  Duration: ~15 min               │
└──────────────────────────────┴──────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                   STAGE 3: TESTING (Sequential)                  │
├─────────────────────┬───────────────────┬───────────────────────┤
│  L1 Tests           │  L2 Tests         │  L1+L2 with Valgrind  │
│  • Unit tests       │  • Integration    │  • Memory leaks       │
│  • Fast execution   │  • Thunder runtime│  • Invalid access     │
│  • Coverage data    │  • Coverage data  │  • File descriptors   │
│  Duration: ~5 min   │  Duration: ~10min │  Duration: ~30 min    │
└─────────────────────┴───────────────────┴───────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                   STAGE 4: ANALYSIS (Parallel)                   │
├─────────────────────┬───────────────────┬───────────────────────┤
│  Coverage Analysis  │  Test Reports     │  Valgrind Analysis    │
│  • lcov processing  │  • Parse results  │  • Check leaks        │
│  • Filter coverage  │  • Failure summary│  • Report violations  │
│  • Generate HTML    │  • Trend analysis │  Duration: ~3 min     │
│  Duration: ~5 min   │  Duration: ~2 min │                       │
└─────────────────────┴───────────────────┴───────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                   STAGE 5: QUALITY GATES                         │
│  • All tests passed (L1 + L2)                                   │
│  • No Valgrind errors                                           │
│  • Coverage ≥ minimum thresholds (per ADR-002)                  │
│  • No high-severity Coverity defects                            │
│  Duration: ~1 min                                               │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                   STAGE 6: REPORTING                             │
│  • Upload artifacts (coverage, reports, logs)                   │
│  • Post PR comments with summary                                │
│  • Update status checks                                         │
│  • Notify on failures                                           │
│  Duration: ~2 min                                               │
└─────────────────────────────────────────────────────────────────┘
```

**Total Pipeline Duration:** ~45-60 minutes (with caching)

### Workflow Organization

#### Core Workflows

##### 1. L1-tests.yml
**Purpose:** Unit test execution and coverage

**Trigger:**
```yaml
on:
  push:
    branches: [ main, develop, 'sprint/**', 'release/**', 'topic/**' ]
  pull_request:
    branches: [ main, develop, 'sprint/**', 'release/**', 'topic/**' ]
```

**Strategy Matrix:**
```yaml
strategy:
  matrix:
    compiler: [ gcc ]
    coverage: [ with-coverage ]
    # Clang excluded for coverage builds (gcc only)
```

**Key Steps:**
1. Cache Thunder/ThunderInterfaces builds
2. Install dependencies (GStreamer, boost, protobuf, etc.)
3. Build Thunder framework stack
4. Build entservices-cpc-apis
5. Build mock library
6. Build rdkservices-cpc plugins
7. Build test framework executable
8. Setup test environment (directories, files)
9. Run tests without Valgrind
10. Run tests with Valgrind
11. Generate coverage reports
12. Upload artifacts

**Artifacts:**
- Coverage HTML reports
- Valgrind logs
- Test result JSON files (with/without Valgrind)

##### 2. L2-tests.yml
**Purpose:** Integration test execution and coverage

**Similar structure to L1-tests.yml with differences:**
- Thunder runs on port 9998
- Additional test environment setup (USB devices, filesystems)
- L2-specific plugin configurations
- Integration test execution

**Artifacts:**
- Coverage HTML reports
- Valgrind logs
- Test result JSON files

##### 3. coverity_full_scan.yml & coverity_incremental_scan.yml
**Purpose:** Static code analysis with Coverity

**Trigger:**
- Full scan: Weekly scheduled + manual
- Incremental: On PR to main/develop

**Steps:**
1. Build with Coverity wrapper: `cov-build`
2. Analyze results: `cov-analyze`
3. Commit defects to Coverity server
4. Generate report
5. Fail on high-severity defects

**Quality Gates:**
- No new high-severity defects
- Total defects not increasing

##### 4. json_validator.yml
**Purpose:** Validate plugin JSON configurations

**Trigger:** All pushes and PRs

**Steps:**
1. Install jsonschema validator
2. Validate each plugin's JSON against schema
3. Report validation errors

**Duration:** ~2 minutes (fast feedback)

##### 5. component-release.yml
**Purpose:** Automated component release workflow

**Trigger:** Manual dispatch with version parameter

**Steps:**
1. Validate version format
2. Update CHANGELOG.md
3. Update version files
4. Create Git tag
5. Build release artifacts
6. Create GitHub release
7. Notify stakeholders

##### 6. update-changelog-and-api-version.yml
**Purpose:** Automated changelog and versioning

**Trigger:** Scheduled or manual

**Steps:**
1. Parse commit messages
2. Generate changelog entries
3. Update API version if needed
4. Create PR with changes

### Build Optimization Strategies

#### Caching Strategy

**Thunder Framework Cache:**
```yaml
- name: Set up cache
  uses: actions/cache@v3
  with:
    path: |
      thunder/build/Thunder
      thunder/build/entservices-apis
      thunder/build/ThunderTools
      thunder/install
      !thunder/install/usr/lib/wpeframework/plugins  # Exclude plugins
      !thunder/install/usr/include/gmock              # Exclude test libs
      !thunder/install/usr/include/gtest
    key: ${{ runner.os }}-${{ env.THUNDER_REF }}-${{ env.INTERFACES_REF }}-4
```

**Cache Benefits:**
- Reduces build time from ~30 min to ~15 min
- Cached across workflow runs
- Invalidated when Thunder version changes

**Cache Hit Rate Target:** >80%

#### Parallel Execution

**Matrix Builds:**
```yaml
strategy:
  matrix:
    compiler: [ gcc, clang ]
    coverage: [ with-coverage, without-coverage ]
  exclude:
    - compiler: clang
      coverage: with-coverage  # Clang doesn't support gcov
```

**Benefit:** Run multiple configurations simultaneously

#### Conditional Steps

**Skip expensive steps when not needed:**
```yaml
- name: Run unit tests with valgrind
  if: ${{ !env.ACT }}  # Skip in local Act runs
  run: valgrind --tool=memcheck RdkServicesL1Test
```

**Skip on draft PRs:**
```yaml
- name: Coverage analysis
  if: ${{ !github.event.pull_request.draft }}
  run: lcov -c -o coverage.info
```

### Resource Management

#### Runner Selection

**Self-Hosted Runners:**
```yaml
runs-on: comcast-ubuntu-latest
```

**Specifications:**
- OS: Ubuntu 22.04
- CPU: 8+ cores
- RAM: 16+ GB
- Disk: 100+ GB SSD
- Network: High-bandwidth for artifact upload

**Benefits:**
- Faster than GitHub-hosted runners
- Persistent cache between runs
- Custom tool installations
- Cost-effective for high-volume usage

#### Concurrency Control

**Prevent redundant builds:**
```yaml
concurrency:
  group: ${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true
```

**Benefit:** Cancel old builds when new commits pushed

#### Artifact Management

**Retention Policy:**
```yaml
- name: Upload artifacts
  uses: actions/upload-artifact@v4
  with:
    name: artifacts-L1-rdkservices-cpc
    retention-days: 30  # Auto-delete after 30 days
```

**Artifact Cleanup:**
- Coverage reports: 30 days
- Test results: 90 days
- Release artifacts: Indefinite
- Debug logs: 7 days

### Quality Gates

#### Test Success Gate
```yaml
- name: Check test results
  run: |
    if [ ! -f rdkL1TestResults.json ]; then
      echo "Test results not found!"
      exit 1
    fi
    
    failures=$(jq '.failures' rdkL1TestResults.json)
    if [ "$failures" != "0" ]; then
      echo "Tests failed: $failures failures"
      exit 1
    fi
```

#### Coverage Gate
```yaml
- name: Check coverage threshold
  run: |
    coverage=$(lcov --summary coverage.info | grep lines | awk '{print $2}' | sed 's/%//')
    threshold=80
    
    if (( $(echo "$coverage < $threshold" | bc -l) )); then
      echo "Coverage $coverage% is below threshold $threshold%"
      exit 1
    fi
```

#### Valgrind Gate
```yaml
- name: Check valgrind results
  run: |
    if grep -q "definitely lost" valgrind_log; then
      echo "Memory leaks detected!"
      cat valgrind_log
      exit 1
    fi
```

#### Coverity Gate
```yaml
- name: Check Coverity defects
  run: |
    high_severity=$(cov-analyze --dir idir | grep "High Impact" | wc -l)
    if [ "$high_severity" -gt "0" ]; then
      echo "High severity defects found: $high_severity"
      exit 1
    fi
```

### PR Integration

#### Status Checks
Required status checks on PRs:
- ✅ JSON Validation
- ✅ L1 Tests (gcc, with-coverage)
- ✅ L2 Tests (gcc, with-coverage)
- ✅ Coverage >= 80%
- ✅ Coverity Incremental Scan

#### Automated PR Comments

**Coverage Comment:**
```markdown
## Coverage Report

| Metric | Value | Change |
|--------|-------|--------|
| Line Coverage | 85.2% | +1.3% 📈 |
| Function Coverage | 88.5% | -0.2% 📉 |
| Branch Coverage | 78.9% | +0.5% 📈 |

[View detailed report](https://artifacts.github.com/...)

✅ Coverage meets minimum requirements
```

**Test Results Comment:**
```markdown
## Test Results

### L1 Tests
- ✅ 145 tests passed
- ⏱️ Duration: 4m 32s
- 🧪 Coverage: 85.2%

### L2 Tests
- ✅ 58 tests passed
- ⏱️ Duration: 9m 18s
- 🧪 Coverage: 81.7%

### Valgrind
- ✅ No memory leaks detected
- ✅ No invalid memory access
```

#### Auto-Merge Criteria
Enable auto-merge when:
1. All status checks pass
2. Approved by 2+ reviewers
3. No unresolved conversations
4. No merge conflicts
5. Not a draft PR

### Notification Strategy

#### Success Notifications
- ✅ Status check update (GitHub)
- No additional notifications (reduce noise)

#### Failure Notifications
- ❌ Status check update (GitHub)
- 📧 Email to commit author
- 💬 Slack notification to #rdkservices-cpc channel
- 📝 PR comment with failure details

#### Security Notifications
- 🔒 High-severity Coverity defects → Security team
- 🔒 Dependency vulnerabilities → Security team

### Environment Configuration

#### Environment Variables

**Build Configuration:**
```yaml
env:
  BUILD_TYPE: Debug
  THUNDER_REF: "R4.4.1"
  INTERFACES_REF: "develop"
  CMAKE_BUILD_PARALLEL_LEVEL: 8
```

**Secrets Management:**
```yaml
env:
  AUTOMATICS_UNAME: ${{ secrets.AUTOMATICS_UNAME }}
  AUTOMATICS_PASSCODE: ${{ secrets.AUTOMATICS_PASSCODE }}
  RDKE_GITHUB_TOKEN: ${{ secrets.RDKE_GITHUB_TOKEN }}
  COVERITY_TOKEN: ${{ secrets.COVERITY_TOKEN }}
```

**Secret Rotation:**
- Quarterly rotation for service accounts
- Immediate rotation on suspected compromise
- Audit log for secret access

#### Test Environment Setup

**L1 Test Environment:**
```bash
# Minimal setup - mocks handle dependencies
mkdir -p /opt/persistent
mkdir -p /tmp/rdkservicestore
```

**L2 Test Environment:**
```bash
# Comprehensive setup for integration tests
sudo mkdir -p -m 777 \
  /opt/persistent \
  /opt/secure/persistent \
  /opt/www/authService \
  /tmp/persistent/rdkservicestore \
  /opt/drm \
  /run/media/sda1 \
  /run/sda2 \
  /dev/disk/by-id

# Create device nodes
mknod /dev/sda c 240 0
mknod /dev/sda1 c 240 0

# Create symlinks
ln -s /dev/sda /dev/disk/by-id/usb-Generic_Flash_Disk_B32FD507-0

# Create test files
echo "{}" > /etc/authService.conf
echo "1.0.0" > /version.txt
```

### Monitoring and Metrics

#### Pipeline Metrics

**Tracked Metrics:**
- Pipeline success rate (target: >95%)
- Average pipeline duration (target: <60 min)
- Cache hit rate (target: >80%)
- Artifact storage usage
- Runner utilization
- Flaky test rate (target: <1%)

**Dashboards:**
- GitHub Actions Insights
- Custom Grafana dashboard for detailed metrics
- Weekly team report with trends

#### Performance Optimization

**Benchmarks:**
| Stage | Current | Target | Optimization |
|-------|---------|--------|--------------|
| Build | 15 min | 10 min | Better caching |
| L1 Tests | 5 min | 4 min | Parallel execution |
| L2 Tests | 10 min | 8 min | Optimize fixtures |
| Valgrind | 30 min | 25 min | Selective runs |
| Coverage | 5 min | 3 min | Incremental analysis |

### Disaster Recovery

#### Pipeline Failure Scenarios

**Scenario 1: Runner Unavailable**
- **Detection:** Workflow queued for >15 min
- **Action:** Alert infrastructure team
- **Fallback:** Use GitHub-hosted runners
- **Recovery Time:** <30 min

**Scenario 2: Dependency Unavailable**
- **Example:** Thunder repo unreachable
- **Detection:** Checkout step fails
- **Action:** Use cached version, alert team
- **Fallback:** Skip optional dependencies
- **Recovery Time:** <1 hour

**Scenario 3: Test Infrastructure Down**
- **Example:** Coverity server unreachable
- **Detection:** Upload step fails
- **Action:** Continue pipeline, defer scan
- **Fallback:** Run scan in next build
- **Recovery Time:** Next successful build

#### Rollback Procedures

**Failed Merge to Main:**
1. Immediately revert merge commit
2. Re-run CI on reverted state
3. Investigate failure in separate branch
4. Fix and re-submit PR

**Bad Release Tag:**
1. Delete tag from GitHub
2. Create hotfix branch
3. Fix issue and create new tag
4. Update dependent systems

### Security Considerations

#### Code Security
- **SAST:** Coverity static analysis
- **Dependency Scanning:** GitHub Dependabot
- **Secret Scanning:** GitHub secret scanning
- **License Compliance:** Automated license checking

#### Build Security
- **Runner Isolation:** Each build in clean environment
- **Artifact Signing:** Sign release artifacts
- **Access Control:** Limited write access to main branches
- **Audit Logging:** All pipeline actions logged

#### Supply Chain Security
- **Dependency Pinning:** Pin Thunder versions
- **Hash Verification:** Verify downloaded dependencies
- **Provenance:** Generate SLSA provenance for releases

## Consequences

### Positive
1. **Quality Assurance:** Comprehensive testing prevents regressions
2. **Fast Feedback:** Parallel execution provides quick results
3. **Automation:** Reduces manual testing burden
4. **Visibility:** Clear metrics and reporting
5. **Consistency:** Same tests run for everyone
6. **Reliability:** Quality gates ensure standards
7. **Scalability:** Can handle increasing complexity

### Negative
1. **Complexity:** Multi-stage pipeline requires maintenance
2. **Resource Usage:** Consumes significant CI resources
3. **Build Time:** Full pipeline takes 45-60 minutes
4. **Debugging:** Failures in CI harder to debug than local
5. **Cost:** Self-hosted runners have infrastructure costs

### Neutral
1. **Learning Curve:** New developers need training
2. **Maintenance:** Workflows require periodic updates
3. **Dependencies:** Relies on external services (GitHub Actions)

## Alternatives Considered

### Alternative 1: Jenkins Pipeline
- **Pros:** More control, plugins ecosystem
- **Cons:** Infrastructure overhead, maintenance burden
- **Rejected:** GitHub Actions better integrated

### Alternative 2: GitLab CI
- **Pros:** Built-in features, good caching
- **Cons:** Would require migration from GitHub
- **Rejected:** Already on GitHub

### Alternative 3: Minimal CI (Tests Only)
- **Pros:** Simple, fast
- **Cons:** No coverage, no static analysis, poor quality
- **Rejected:** Insufficient for production quality

### Alternative 4: External CI Service (CircleCI, Travis)
- **Pros:** Specialized features
- **Cons:** Additional service dependency, cost
- **Rejected:** GitHub Actions sufficient

## Implementation Plan

### Phase 1: Foundation (Weeks 1-2)
- [x] L1 test workflow
- [x] L2 test workflow
- [x] JSON validation workflow
- [ ] Enhance caching strategy
- [ ] Add quality gates

### Phase 2: Enhancement (Weeks 3-4)
- [ ] Implement coverage gates
- [ ] Add PR comment automation
- [ ] Set up monitoring dashboard
- [ ] Optimize build times

### Phase 3: Advanced Features (Weeks 5-8)
- [ ] Implement auto-merge
- [ ] Add performance benchmarking
- [ ] Set up notification system
- [ ] Create disaster recovery procedures

### Phase 4: Optimization (Ongoing)
- [ ] Continuous performance tuning
- [ ] Regular metric reviews
- [ ] Workflow refinements
- [ ] Tool upgrades

## References
- GitHub Actions Documentation: https://docs.github.com/en/actions
- Coverity Documentation: https://scan.coverity.com/
- SLSA Supply Chain Security: https://slsa.dev/
- Current Workflows: `.github/workflows/`

## Revision History
- **Version 1.0** (December 15, 2025): Initial CI/CD architecture proposal

## Related ADRs
- ADR-001: L1 and L2 Testing Strategy
- ADR-002: Test Coverage Requirements
- ADR-003: Mock Management Strategy
