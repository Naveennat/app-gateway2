# app-gateway2

app-gateway2 provides Thunder (WPEFramework) plugin sources along with a lightweight Node.js preview server so the preview system can start and display a running page.

What this repo contains
- C++/CMake sources for Thunder plugins and related components.
- A Node.js HTTP server (server.js) that the preview system uses to start the container reliably.
- A build helper script (scripts/start.sh) to configure and build native components locally if desired.

How to start (preview)
- Using npm (recommended):
  - npm start
- Equivalent direct invocation:
  - node server.js
  - or ./run.sh

Port
- The server listens on the PORT environment variable (default: 3001).
- Example:
  - PORT=3001 npm start

Endpoints
- /           -> Simple HTML page with basic info and useful links
- /healthz    -> JSON health response
- /info       -> Runtime info (Node version, platform, etc.)
- /logs/build -> Tails .init/build.log if present
- /logs/tests -> Tails .init/tests.log if present

Preview system integration
- package.json includes a valid start script:
  - "start": "node server.js"
- Procfile is provided for platforms that use it:
  - web: npm start
- run.sh is a simple wrapper that also executes server.js

Building native (C++/CMake) components (optional)
- To configure and build locally, run:
  - npm run start:build
    - (Equivalent: bash scripts/start.sh)
- What scripts/start.sh does:
  - Configures CMake into build-local/
  - Builds targets with parallel jobs
  - Optionally runs a known local test binary once if available
  - Streams build/test logs to .init/ for inspection
- After running the above, you can access:
  - http://localhost:3001/logs/build
  - http://localhost:3001/logs/tests

Thunder (WPEFramework) integration notes
- Produced artifacts are Thunder plugins designed to be loaded by a running Thunder instance.
- Example plugin configuration: configs/plugins/OttServices.json.example
- Typical Thunder JSON-RPC endpoint: http://127.0.0.1:9998/jsonrpc
- Ensure plugins are present and ACTIVATED in the Controller for JSON-RPC calls to succeed.

Troubleshooting the plugin runtime (Thunder)
- If a JSON-RPC call returns ERROR_UNAVAILABLE, verify:
  - The plugin exists in Controller and is ACTIVATED.
  - The Thunder instance is reachable on the configured port (default 9998) and path (/jsonrpc by default).
- Example checks:
  - Plugin status:
    curl -s -H "Content-Type: application/json" \
      -d '{"jsonrpc":"2.0","id":1,"method":"Controller.1.status@OttServices"}' \
      http://127.0.0.1:9998/jsonrpc
  - Activate plugin:
    curl -s -H "Content-Type: application/json" \
      -d '{"jsonrpc":"2.0","id":2,"method":"Controller.1.activate","params":{"callsign":"OttServices"}}' \
      http://127.0.0.1:9998/jsonrpc

Summary
- The container now has a clear, platform-friendly start command (npm start) and a Procfile that points to the same server entry.
- The Node.js server provides readiness endpoints and log tailing to aid diagnostics.
- Use scripts/start.sh if you also want to build and inspect the native components locally.
