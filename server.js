/**
 * Minimal Express static server for app-gateway2.
 * Serves files from the ./public directory and respects the PORT environment variable.
 */
'use strict';

const path = require('path');
const express = require('express');

const PUBLIC_DIR = path.join(__dirname, 'public');

// PUBLIC_INTERFACE
function createServer() {
  /** Create and configure an Express server to serve the static frontend for app-gateway2. */
  const app = express();

  // Serve static assets
  app.use(
    express.static(PUBLIC_DIR, {
      extensions: ['html'],
      index: 'index.html',
      maxAge: '1h'
    })
  );

  // Health check endpoint
  app.get('/health', (_req, res) => {
    res.json({ status: 'ok' });
  });

  // SPA-style fallback to index.html
  app.get('*', (_req, res) => {
    res.sendFile(path.join(PUBLIC_DIR, 'index.html'));
  });

  return app;
}

// If executed directly, start the HTTP server
if (require.main === module) {
  const app = createServer();
  const port = normalizePort(process.env.PORT || '3000'); // uses PORT if provided by preview system
  const host = '0.0.0.0';
  app.listen(port, host, () => {
    console.log(`app-gateway2 running at http://${host}:${port}`);
  });
}

/**
 * Normalize the provided port value into a number or string (for named pipes).
 * Falls back to 3000 when an invalid value is supplied.
 */
function normalizePort(val) {
  const port = parseInt(val, 10);
  if (Number.isNaN(port)) return val; // named pipe
  if (port >= 0) return port; // valid port number
  return 3000;
}

// Exported for potential tests or reuse elsewhere
module.exports = { createServer };
