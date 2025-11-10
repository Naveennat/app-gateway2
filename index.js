'use strict';

/**
 * app-gateway2 entrypoint
 * This file intentionally exists so preview/build systems that look for a conventional
 * Node entry (index.js) can detect and run the service without special logic.
 * It delegates to the existing server.js implementation.
 */

// Basic process-level error logging to avoid silent exits
process.on('unhandledRejection', (err) => {
  console.error('[app-gateway2] Unhandled Promise Rejection:', err);
});
process.on('uncaughtException', (err) => {
  console.error('[app-gateway2] Uncaught Exception:', err);
});

console.log(`[app-gateway2] Node version: ${process.version}`);

// Requiring server.js starts the server (server.js calls app.listen)
require('./server');
