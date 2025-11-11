'use strict';

/**
 * Minimal HTTP server for app-gateway2 preview.
 * Provides a reliable long-running process the preview system can launch.
 * Listens on process.env.PORT (default 3001) and serves basic health/info and log tails if present.
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const os = require('os');

const ROOT_DIR = __dirname;
const LOG_DIR = path.join(ROOT_DIR, '.init');

/**
 * Read the tail end of a file up to a maximum number of bytes.
 * @param {string} filePath Absolute path to file.
 * @param {number} maxBytes Max bytes to return from end of file.
 * @returns {string} Tail content or a friendly message if not found.
 */
function readTail(filePath, maxBytes = 64 * 1024) {
  try {
    const stats = fs.statSync(filePath);
    const size = stats.size;
    const fd = fs.openSync(filePath, 'r');
    const start = Math.max(0, size - maxBytes);
    const length = size - start;
    const buffer = Buffer.alloc(length);
    fs.readSync(fd, buffer, 0, length, start);
    fs.closeSync(fd);
    return buffer.toString('utf8');
  } catch (err) {
    return `Log not available (${filePath}): ${err.message}`;
  }
}

/**
 * Render a simple HTML page with basic info.
 */
function renderIndex(port) {
  const healthUrl = '/healthz';
  const infoUrl = '/info';
  const buildLogUrl = '/logs/build';
  const testsLogUrl = '/logs/tests';
  return `<!doctype html>
<html>
  <head>
    <meta charset="utf-8" />
    <title>app-gateway2 preview</title>
    <style>
      body { font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif; margin: 2rem; line-height: 1.5; }
      code { background: #f4f4f4; padding: 2px 4px; border-radius: 4px; }
      .box { background: #fafafa; border: 1px solid #e6e6e6; padding: 1rem; border-radius: 6px; }
      a { color: #0366d6; text-decoration: none; }
    </style>
  </head>
  <body>
    <h1>app-gateway2 preview is running</h1>
    <div class="box">
      <p>This container provides a lightweight HTTP server so the preview system can start and display a page.</p>
      <ul>
        <li>Port: <code>${port}</code></li>
        <li>Health: <a href="${healthUrl}">${healthUrl}</a></li>
        <li>Info: <a href="${infoUrl}">${infoUrl}</a></li>
        <li>Build log (if present): <a href="${buildLogUrl}">${buildLogUrl}</a></li>
        <li>Tests log (if present): <a href="${testsLogUrl}">${testsLogUrl}</a></li>
      </ul>
      <p>To build native components, run <code>npm run start:build</code> (requires CMake and a C++ toolchain).</p>
    </div>
  </body>
</html>`;
}

/**
 * Build the HTTP request handler.
 */
function requestHandler(req, res) {
  const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
  const pathName = url.pathname;

  if (pathName === '/' || pathName === '/index.html') {
    const port = process.env.PORT || 3000;
    const html = renderIndex(port);
    res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
    res.end(html);
    return;
  }

  if (pathName === '/healthz' || pathName === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'ok' }));
    return;
  }

  if (pathName === '/info') {
    const info = {
      name: 'app-gateway2',
      node: process.version,
      platform: process.platform,
      arch: process.arch,
      cwd: process.cwd(),
      pid: process.pid,
      env: {
        PORT: process.env.PORT || null,
      },
    };
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify(info, null, 2));
    return;
  }

  if (pathName === '/logs/build') {
    const logPath = path.join(LOG_DIR, 'build.log');
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end(readTail(logPath));
    return;
  }

  if (pathName === '/logs/tests') {
    const logPath = path.join(LOG_DIR, 'tests.log');
    res.writeHead(200, { 'Content-Type': 'text/plain; charset=utf-8' });
    res.end(readTail(logPath));
    return;
  }

  res.writeHead(404, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify({ error: 'Not Found', path: pathName }));
}

// PUBLIC_INTERFACE
function createServer() {
  /**
   * Create and return the preview HTTP server.
   * This is the public interface for testing or custom bootstraps.
   * @returns {http.Server} Node HTTP server instance.
   */
  return http.createServer(requestHandler);
}

if (require.main === module) {
  const port = parseInt(process.env.PORT || '3000', 10);
  const host = '0.0.0.0';

  // Ensure log dir exists if logs will be tailed
  try {
    fs.mkdirSync(LOG_DIR, { recursive: true });
  } catch {
    // ignore
  }

  const server = createServer();
  server.listen(port, host, () => {
    // eslint-disable-next-line no-console
    console.log(`[app-gateway2] Preview server listening on http://${host}:${port}`);
  });

  // Graceful shutdown
  const shutdown = (sig) => () => {
    // eslint-disable-next-line no-console
    console.log(`[app-gateway2] Received ${sig}, shutting down...`);
    server.close(() => process.exit(0));
    setTimeout(() => process.exit(0), 2000).unref();
  };
  process.on('SIGINT', shutdown('SIGINT'));
  process.on('SIGTERM', shutdown('SIGTERM'));
}

module.exports = { createServer };
