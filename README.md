# app-gateway2

Minimal preview server for the app-gateway2 repository.

Detected framework
- Node.js lightweight HTTP server (no external dependencies; uses Node's built-in http module)

How to run
- npm run build  -> No-op (prints a message). Exists so preview/CI build steps succeed.
- npm start      -> Starts the production-capable server (binds to 0.0.0.0 and respects PORT).
- npm run dev    -> Starts the server in the same way as start (convenience for local runs).
- Alternatively: node server.js or node index.js

Environment variables
- PORT: Port to listen on (default: 3000)
- HOST: Host interface to bind (default: 0.0.0.0). The server binds to 0.0.0.0 by default for container previews.

Endpoints
- GET /        -> Simple HTML page: "app-gateway2 Preview" and links to local docs/reports if present
- GET /health  -> {"status":"ok"}
- GET /docs    -> Serves files from ./docs if available
- GET /reports -> Serves files from ./Supporting_Files/Badger_Test_Results/HTML_Report if available

Procfile
- A Procfile is present with: `web: npm start` for platforms that use it.

Notes
- No external dependencies are required. The server uses Node's built-in http module.
- The preview system requires that a start command exists and the app listens on the provided PORT and 0.0.0.0.
- Dockerfile is included for containerized runs and exposes port 3000.

---

Original README (retained for reference)

OttServices Thunder JSON-RPC plugin

Summary
- JSON-RPC method OttServices.1.getpermissions previously returned ERROR_UNAVAILABLE if the plugin was not active. This is the Thunder controller’s generic response when a plugin callsign exists but is not in the ACTIVATED state.
- Fixes added:
  - Explicit JSON-RPC version registration (1.0.0) for the OttServices handler to improve discoverability in docs and tools.
  - Added a camelCase alias OttServices.1.getPermissions for better client compatibility.
- Root cause most often: OttServices plugin not configured/activated on the Thunder instance serving port 9998.

Why ERROR_UNAVAILABLE happened
- Thunder routes JSON-RPC requests to the plugin by callsign and version. If the plugin exists but is not activated, Controller::Invoke returns ERROR_UNAVAILABLE with a message like “Service is not active”.
- Calling OttServices.1.getpermissions against a Thunder instance that does not have the plugin activated will thus return error code ERROR_UNAVAILABLE (typically code 2).

Quick verification and activation steps
1) Ensure Thunder listens on port 9998 and exposes /jsonrpc
- In /etc/WPEFramework/config.json set:
  {
    "port": 9998,
    "jsonrpc": "jsonrpc",
    "binding": "0.0.0.0"    // if curling externally; use 127.0.0.1 for local-only
  }
- Restart Thunder (WPEFramework) after configuration changes.

2) Check if OttServices plugin is present and its state
- Query plugin summary:
  curl -s http://127.0.0.1:9998/Service/Controller
- Query specific plugin status via JSON-RPC:
  curl -s -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":1,"method":"Controller.1.status@OttServices"}' \
    http://127.0.0.1:9998/jsonrpc

3) If the plugin is not active, activate it
- Via REST:
  curl -X PUT http://127.0.0.1:9998/Service/Controller/Activate/OttServices
- Or via JSON-RPC:
  curl -s -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":2,"method":"Controller.1.activate","params":{"callsign":"OttServices"}}' \
    http://127.0.0.1:9998/jsonrpc

4) Test JSON-RPC methods
- Ping (basic readiness):
  curl -s -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":3,"method":"OttServices.1.ping","params":{"message":"hi"}}' \
    http://127.0.0.1:9998/jsonrpc
- getpermissions (lowercase):
  curl -s -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":4,"method":"OttServices.1.getpermissions","params":{"appId":"com.example.app"}}' \
    http://127.0.0.1:9998/jsonrpc
- getPermissions (camelCase alias also supported):
  curl -s -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":5,"method":"OttServices.1.getPermissions","params":{"appId":"com.example.app"}}' \
    http://127.0.0.1:9998/jsonrpc

If you still receive ERROR_UNAVAILABLE
- Confirm the plugin is ACTIVATED in Controller:
  curl -s -H "Content-Type: application/json" \
    -d '{"jsonrpc":"2.0","id":6,"method":"Controller.1.status@OttServices"}' \
    http://127.0.0.1:9998/jsonrpc
- If the plugin is DEACTIVATED, re-activate it. If the plugin is not listed, you need to register the plugin (see next section).
- Check Thunder logs and plugin logs for messages:
  - Look for “OttServices: constructed” and “OttServices: initialized successfully”.
  - Each JSON-RPC call logs the method and result code.

Registering the plugin (plugin configuration)
Place a plugin configuration JSON file for OttServices in the Thunder plugins config directory. By default this is /etc/WPEFramework/plugins. A working example is provided in this repo at:
  app-gateway2/configs/plugins/OttServices.json.example

Typical contents:
{
  "callsign": "OttServices",
  "locator": "libOttServices.so",
  "classname": "OttServices",
  "startmode": "Activated",
  "startuporder": 10,
  "configuration": {
    "PermissionsEndpoint": "thor-permission.svc.thor.comcast.com:443",
    "UseTls": true,
    "root": { "mode": "Off" }
  }
}

Notes:
- startmode: "Activated" ensures the plugin is started at Thunder boot.
- If your build produces a different plugin .so name or uses a shared monolithic plugin library, adjust locator accordingly.
- The configuration fields PermissionsEndpoint and UseTls are read by the implementation at startup.

Error reference
- ERROR_UNAVAILABLE: The plugin is configured but is not in the ACTIVATED state, or a dependent resource could not be reached at call time (e.g., auth metadata for updatepermissionscache). Use the Controller to activate the plugin and re-try.
- ERROR_BAD_REQUEST: Missing/invalid parameters (e.g., getpermissions without an appId).
- ERROR_NONE: Success.

Readiness and logging
- OttServices registers JSON-RPC methods at construction and logs “OttServices: JSON-RPC methods registered”.
- Ping endpoint is provided for a lightweight readiness check.
- All endpoints log inputs and results. Use Thunder/WPE logs to diagnose environment issues (e.g., permissions service reachability or auth service linkage).

Security tokens
- If your Thunder instance enforces tokens, ensure to pass a valid token header or disable security in the Controller configuration for local testing.

Port and routing
- If your environment uses a different port (e.g., 80), adjust curl URLs accordingly.
- /jsonrpc is the default JSON-RPC endpoint path and is configurable via the jsonrpc field in /etc/WPEFramework/config.json.

Troubleshooting checklist
- [ ] Thunder is listening on the expected port and /jsonrpc is reachable.
- [ ] OttServices plugin is present in the Controller plugin list.
- [ ] The plugin is ACTIVATED (not DEACTIVATED/UNAVAILABLE/HIBERNATED).
- [ ] The endpoint method and casing are correct: getpermissions or getPermissions.
- [ ] Required params are provided (e.g., appId).
- [ ] Check logs for any implementation-specific errors (auth service, gRPC permissions service, etc.).
