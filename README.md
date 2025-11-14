# app-gateway2

Minimal Express web server for local development and preview.

## Prerequisites

- Node.js (v14 or above)
- npm

## Setup

```sh
npm install
```
(To install dependencies.)

## Running

```sh
npm start
```
_By default binds to `PORT=3000` (or whatever is set in `.env`)._

- Visit [http://localhost:3000](http://localhost:3000)
- Health check (returns 200 OK): [http://localhost:3000/healthz](http://localhost:3000/healthz)

## Environment Variables

Create a `.env` file if needed with:
```
PORT=3000
```

## Files

- `server.js` &mdash; Express app, static file server, health endpoint.
- `public/index.html` &mdash; Placeholder content.
- `.env.example` &mdash; Example env settings.
- `package.json` &mdash; NPM metadata, scripts, dependencies.

---
This scaffold is intended for development/preview only.
