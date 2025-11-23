'use strict';

/**
 * app-gateway2 server entrypoint
 * - Binds to process.env.PORT and process.env.HOST
 * - Exposes /health route for health checks
 */

const express = require('express');

/**
 * PUBLIC_INTERFACE
 * Create and configure the Express application for app-gateway2.
 * Returns an Express app instance.
 * The app exposes:
 *  - GET /health: returns 'ok' for health checks
 *  - GET /: basic info route
 * @returns {import('express').Express} Configured Express app
 */
function createServerApp() {
  const app = express();

  // Basic middleware
  app.use(express.json());

  /**
   * GET /health
   * Health check endpoint; returns HTTP 200 OK with 'ok'.
   * Summary: Health check
   * Description: Returns 'ok' and HTTP 200 for liveness checks.
   * Responses:
   *   200: ok
   */
  app.get('/health', (req, res) => {
    res.status(200).send('ok');
  });

  /**
   * GET /
   * Simple home route to confirm the service is running.
   * Responses:
   *   200: JSON with service status details
   */
  app.get('/', (req, res) => {
    res.status(200).json({
      service: 'app-gateway2',
      status: 'ok',
      timestamp: new Date().toISOString(),
      uptime: process.uptime()
    });
  });

  return app;
}

/**
 * PUBLIC_INTERFACE
 * Starts the HTTP server using environment variables HOST and PORT.
 * PORT defaults to 3000; HOST defaults to '0.0.0.0'.
 * @returns {import('http').Server} The Node HTTP server
 */
function startServer() {
  const PORT = process.env.PORT ? parseInt(process.env.PORT, 10) : 3000;
  const HOST = process.env.HOST || '0.0.0.0';
  const app = createServerApp();

  const server = app.listen(PORT, HOST, () => {
    // eslint-disable-next-line no-console
    console.log(`[app-gateway2] Listening on http://${HOST}:${PORT}`);
  });

  server.on('error', (err) => {
    // eslint-disable-next-line no-console
    console.error('[app-gateway2] Server error:', err);
  });

  // Graceful shutdown
  const shutdown = () => {
    // eslint-disable-next-line no-console
    console.log('[app-gateway2] Shutting down...');
    server.close(() => {
      process.exit(0);
    });
    // Force exit if not closed after timeout
    setTimeout(() => process.exit(1), 10000).unref();
  };
  process.on('SIGTERM', shutdown);
  process.on('SIGINT', shutdown);

  return server;
}

// If executed directly via `node server.js`, start the server
if (require.main === module) {
  startServer();
}

module.exports = {
  createServerApp,
  startServer,
};
