# app-gateway2

Minimal start configuration enabling the Kavia preview system to run this repository as a web container.

## Quick start

- npm run start

This starts a zero-dependency Node static server that:
- Serves the docs/ directory if present; otherwise serves the repository root.
- Binds to 0.0.0.0 and listens on the PORT environment variable (default 3000).
- Provides a basic directory listing when no index.html is present.

No build step or dependencies are required.

## Notes

- A Procfile is included for platforms that recognize it:
  - `web: node scripts/server.js`
- The start server uses only Node core modules and requires Node.js >= 16.
- To change the port, set the `PORT` environment variable.
