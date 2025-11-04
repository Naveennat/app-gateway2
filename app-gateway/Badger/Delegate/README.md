# Badger Metrics Delegate

This delegate implements Money Badger metrics forwarding using the same RPC mechanism already used by other Badger delegates. It wires up 7 handlers:

- badger.metricsHandler.metrics Handler (generic)
- badger.metricsHandler.Launch Completed metrics handler
- badger.metricsHandler.user Action Metrics Handler
- badger.metricsHandler.app Action Metrics Handler
- badger.metricsHandler.page View Metrics Handler
- badger.metricsHandler.user Error Metrics Handler
- badger.metricsHandler.Error Metrics Handler

Validation:
- The delegate attempts to validate payloads against Supporting_Files/Badger_helper_files/metrics.json.
- If that file is not a valid JSON schema (e.g., HTML due to SSO/gate), it falls back to structural checks aligned with:
  - Supporting_Files/ripple-eos/src/rpc/eos_metrics_rpc.rs
  - Supporting_Files/ripple-eos/src/model/metrics.rs

Transformation:
- Payloads are built to match the BadgerMetricsHandlerParams structure used by eos_metrics_rpc.rs (eos.metricsHandler).
- Timing values are intentionally ignored.

Emission:
- The delegate emits metrics via WPEFramework::Utils::JSONRPCDirectLink to the EOS metrics method: `eos.metricsHandler`
- The callsign is configurable at build time via EOS_METRICS_CALLSIGN (defaults to "RippleEos").

## Public API

See BadgerMetricsDelegate.h for inline documentation on each public method.

Example usage from within the Badger plugin:

```cpp
#include "Delegate/BadgerMetricsDelegate.h"

// In Initialize():
auto metrics = mDelegate->getMetricsDelegate();

// Launch completed (ready)
std::vector<BadgerMetrics::ParamKV> args;
Core::JSON::Variant v;
v = Core::JSON::Variant(); // set to whatever type you need
args.push_back({"launch_time_ms", Core::JSON::Variant(2500.0)});
metrics->LaunchCompleted(args);

// User action
metrics->UserAction("playButtonClick", {
    {"button", Core::JSON::Variant("play")},
    {"count", Core::JSON::Variant(1.0)}
});

// App action
metrics->AppAction("menuOpen", {});

// Page view
metrics->PageView("home", {});

// User error
metrics->UserError("Connection timeout", true, "NETWORK_TIMEOUT", {});

// App error
metrics->Error("Playback stalled", true, "MEDIA_STALLED", {});
```

Note: The delegate is constructed and exposed by `DelegateHandler`, so you can acquire it via:
```cpp
auto metrics = mDelegate->getMetricsDelegate();
```

## Integration

No start/preview scripts are modified. The delegate is registered in DelegateHandler only, so it’s available to the Badger plugin code without changing external runtime settings. If you need to forward app events into these handlers, do so inside `Badger::HandleAppGatewayRequest` or other appropriate locations.

## Callsign

If your EOS metrics plugin uses a different callsign, define `-DEOS_METRICS_CALLSIGN="YourCallsign"` at compile time or change the default in `BadgerMetricsDelegate.h`.

