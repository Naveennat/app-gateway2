# app-gateway2 Minimal Web Gateway

This directory provides a minimal Express-based HTTP server for health checks and preview environment validation.

## Requirements

- Node.js >= 18

## Usage

- Start in production mode:
  ```
  npm install
  npm start
  ```
  or
  ```
  yarn install
  yarn start
  ```

- Development mode (auto-reload):
  ```
  npm run dev
  ```
  or
  ```
  yarn run dev
  ```

## Health and Endpoints

- Root: open your browser at [http://localhost:3000/](http://localhost:3000/) (or on the port assigned in the `PORT` environment variable).
- Health endpoint: `GET /health` responds with `{"status":"ok"}` for liveness checks.

## Ports and Binding

- The server listens on `HOST=0.0.0.0` and respects `PORT` from the environment (defaults to `3000`).
- Do not hardcode ports; rely on the `PORT` env provided by your runtime.

## Static Content

- If a `public/` or `dist/` directory exists, its contents (such as an `index.html`) will be served automatically at the root endpoint.
- This repository includes a minimal `public/index.html` landing page for convenience.

## Notes

- The actual app logic is not implemented in this minimal setup.
- Keep the start script as `npm start` which runs `node server.js`.
- The server is suitable for preview/health check only.
