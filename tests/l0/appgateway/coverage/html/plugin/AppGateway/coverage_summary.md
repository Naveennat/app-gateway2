# plugin/AppGateway coverage summary (LCOV)

Source report: `app-gateway2/tests/l0/appgateway/coverage/html/plugin/AppGateway/index.html`  
Timestamp: 2026-01-27 21:32:22

## Key metrics
- Lines: **55.9%** (645/1153)
- Functions: **47.3%** (61/129)

## Per-file breakdown
| File | Line % | Lines hit/total | Func % | Func hit/total | Notes |
|---|---:|---:|---:|---:|---|
| AppGateway.cpp | 74.6% | 185/248 | 80.0% | 16/20 | Good coverage. |
| AppGateway.h | 71.4% | 5/7 | 50.0% | 1/2 | Some header logic unexercised. |
| AppGatewayImplementation.cpp | 53.5% | 224/419 | 46.7% | 14/30 | Largest gap by volume. |
| AppGatewayImplementation.h | 78.6% | 33/42 | 88.9% | 8/9 | Strong. |
| AppGatewayResponderImplementation.cpp | 37.5% | 63/168 | 33.3% | 6/18 | Major missing coverage. |
| AppGatewayResponderImplementation.h | 0.0% | 0/80 | 0.0% | 0/30 | Completely uncovered. |
| Module.cpp | 0.0% | 0/1 | 0.0% | 0/2 | Plugin lifecycle not executed. |
| Resolver.cpp | 71.2% | 131/184 | 88.2% | 15/17 | Strong coverage. |
| Resolver.h | 100.0% | 4/4 | 100.0% | 1/1 | Fully covered. |

## Notable gaps / follow-ups
- Exercise responder flows (emit/respond/request/connection-context) to lift coverage in:
  - `AppGatewayResponderImplementation.h/.cpp`
- Add tests for additional AppGatewayImplementation handler paths and error branches.
- If feasible in the test harness, drive full plugin lifecycle registration to cover `Module.cpp`.
