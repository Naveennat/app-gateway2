# app-gateway2

## Quick Start (Preview/Dev)

This repository includes a minimal Node.js preview server for app-gateway2.

**Start the preview server:**
```bash
npm install
npm start
```
- This launches Node.js using `index.js` on port specified by the `PORT` environment variable (default: 3000).
- Server binds to `0.0.0.0` for preview containers.
- No dependencies required (uses Node's built-in http module).

---

Minimal preview server for the app-gateway2 repository.

Detected framework:
- Node.js lightweight HTTP server (no external dependencies; uses Node's built-in http module)

How to run:
- `npm start` – Starts the production-capable server (binds to 0.0.0.0 and respects PORT).
- Alternatively: `node index.js`

Environment variables:
- `PORT`: Port to listen on (default: 3000)
- `HOST`: Host interface to bind (default: 0.0.0.0). The server binds to 0.0.0.0 by default for container previews.

Endpoints:
- GET `/` – Simple HTML page: "app-gateway2 Preview"
- GET `/health` – Returns `{"status":"ok"}`

Procfile:
- A Procfile is present with: `web: npm start` for platforms that use it.

Notes:
- No external dependencies are required. The server uses Node's built-in http module.
- The preview system requires that a start command exists and the app listens on the provided PORT and 0.0.0.0.
- Dockerfile is included for containerized runs and exposes port 3000.

---

Original README (retained for reference)

OttServices Thunder JSON-RPC plugin

(Snipped: see previous version in repo for extended Thunder/JSON-RPC instructions.)
