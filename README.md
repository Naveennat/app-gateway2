# app-gateway2

This is the app-gateway2 service.

## Getting Started

1. Install Dependencies

   ```
   npm install
   ```

2. Set Environment Variables

   Copy `.env.example` to `.env` and edit the values as needed.
   ```
   cp .env.example .env
   ```

   - HOST defaults to 0.0.0.0 (required for most container/preview platforms)
   - PORT defaults to 3000

3. Start the Service

   ```
   npm start
   ```

   This runs `node index.js` using the start script in `package.json`. By default, the service will run on HOST/PORT (default: 0.0.0.0:3000).

### Express App Details

- The runtime entrypoint is `index.js` which delegates to `server.js`.
- The server listens on the `HOST` and `PORT` environment variables (defaults: HOST=0.0.0.0, PORT=3000).
- If you add a file as `public/index.html`, it will be served at the root path `/`. If not present, the root path responds with a basic "app-gateway2 running" message.

### Preview/Platform Integration

- `package.json` defines `"start": "node index.js"`.
- A `Procfile` is included with `web: npm start` to assist platforms that expect Procfile (e.g., Heroku-style).
- A `start.sh` helper is included to prefer `npm start` and fallback to `node index.js`.

## Endpoints

- `/` - Serves static `public/index.html` if present, otherwise returns 'app-gateway2 running'.
- `/health` - Health check endpoint, returns `{ "status": "ok" }`.

## Docker

- Minimal `Dockerfile` provided with `CMD ["node", "index.js"]`.
- Exposes port 3000 and sets `HOST=0.0.0.0` by default.
- Includes a lightweight `HEALTHCHECK` that probes `/health`.

## Requirements

- Node.js (>= 16)
- [express](https://expressjs.com/)

## License

See [LICENSE](../LICENSE) for details.
