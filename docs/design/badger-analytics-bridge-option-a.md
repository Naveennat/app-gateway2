# Badger Metrics via Analytics Bridge (Option A)

Summary:
- Badger metrics are now routed through the shared Analytics bridge `org.rdk.Analytics` (ANALYTICS_PLUGIN_CALLSIGN) using COM-RPC and the `Exchange::IAnalytics::SendEvent` entrypoint.
- Payloads are normalized to the same schema used by FbMetrics (event names and payload shapes), avoiding direct EOS/Ripple JSON-RPC calls.

Implementation details:
- Updated `app-gateway2/app-gateway/Badger/Delegate/BadgerMetricsDelegate.h` to:
  - Build event payloads consistent with FbMetrics:
    - User/App Actions -> `inapp_other_action` with `{ category: "user"|"app", type: "<action>", parameters: {...} }`
    - Page View -> `inapp_page_view` with `{ src_page_id: "<page>" }`
    - Errors -> `app_error` with `{ type, code, description, visible, parameters, third_party_error: true }`
    - Generic/Launch-completed -> `inapp_other_action` with `{ category: "app", type: "launch_completed" }`
  - Flatten Badger args vectors (`[{name, value}]`) into a `parameters` JSON object to match FbMetrics’ schema.
  - Dispatch using the same COM-RPC path as `FbMetrics::SendAnalyticsEvent()`:
    - `eventVersion="3"`, `eventSource="ripple"`, `eventSourceVersion="3.5.0"`
    - `cetList`: empty iterator (Analytics enriches internally)
    - Timestamps: `0` (Analytics adds/adjusts)
    - `appId`: (empty; Badger delegate does not currently receive app context)
- Left FbMetrics logic unchanged (no behavioral changes).
- Added a tiny `.cpp` TU for the delegate to satisfy build systems scanning for `.cpp` sources.

Deviation/Notes:
- Badger delegate does not have access to the Firebolt app context, so `app_session_id`, `app_user_session_id`, `durable_app_id` and `app_version` are omitted. This is acceptable per schema (fields are optional) and does not affect routing via Analytics.
- We intentionally did not factor out `SendAnalyticsEvent` from FbMetrics to avoid any risk of regression; Badger uses an equivalent COM-RPC path internally.

Acceptance alignment:
- Events flow via Analytics: `org.rdk.Analytics`.
- Payloads normalized to match FbMetrics shapes.
- No change to FbMetrics behavior.
- Minimal documentation and TODOs in code comments.
