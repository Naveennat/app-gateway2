/**
 * Minimal zero-dependency static file server for app-gateway2.
 *
 * - Serves the docs/ directory if present; otherwise serves the repository root.
 * - Respects the PORT environment variable (default: 3000).
 * - Binds to 0.0.0.0 to work in containerized environments.
 * - Provides a basic directory listing when index.html is not present.
 *
 * Node.js >= 16 required. Uses only core modules.
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const { URL } = require('url');

/** Map file extensions to content types */
const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.htm': 'text/html; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.mjs': 'application/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.xml': 'application/xml; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.map': 'application/json; charset=utf-8',
  '.wasm': 'application/wasm',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.ico': 'image/x-icon',
  '.webp': 'image/webp',
  '.pdf': 'application/pdf',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.ttf': 'font/ttf',
  '.eot': 'application/vnd.ms-fontobject',
  '.mp4': 'video/mp4',
  '.mp3': 'audio/mpeg',
  '.ogg': 'audio/ogg',
  '.m3u8': 'application/vnd.apple.mpegurl',
  '.ts': 'video/mp2t'
};

function contentTypeFor(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  return MIME_TYPES[ext] || 'application/octet-stream';
}

function isPathInside(testPath, rootPath) {
  const rel = path.relative(rootPath, testPath);
  return !!rel && !rel.startsWith('..') && !path.isAbsolute(rel);
}

function escapeHtml(str) {
  return str
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#39;');
}

function directoryListingHTML(dirFsPath, reqPath, rootFsPath) {
  const entries = fs.readdirSync(dirFsPath, { withFileTypes: true });
  const parts = reqPath.split('/').filter(Boolean);
  const breadcrumbs = ['<a href="/">/</a>'];
  let accum = '';
  for (const p of parts) {
    accum += `/${p}`;
    breadcrumbs.push(`<a href="${accum}/">${escapeHtml(p)}</a>`);
  }
  const bc = breadcrumbs.join(' ');

  const items = entries
    .sort((a, b) => {
      // Folders first, then files, alphabetically
      if (a.isDirectory() && !b.isDirectory()) return -1;
      if (!a.isDirectory() && b.isDirectory()) return 1;
      return a.name.localeCompare(b.name);
    })
    .map((ent) => {
      const slash = ent.isDirectory() ? '/' : '';
      const href = path.posix.join(reqPath, ent.name) + slash;
      const safe = escapeHtml(ent.name) + slash;
      return `<li><a href="${href}">${safe}</a></li>`;
    })
    .join('\n');

  return `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8"/>
<title>Index of ${escapeHtml(reqPath)}</title>
<style>
body { font-family: system-ui, -apple-system, Segoe UI, Roboto, Ubuntu, Cantarell, "Helvetica Neue", Arial, "Noto Sans", "Apple Color Emoji", "Segoe UI Emoji", "Segoe UI Symbol", "Noto Color Emoji", sans-serif; margin: 2rem; }
h1 { font-size: 1.25rem; margin-bottom: 1rem; }
ul { line-height: 1.75; }
.code { font-family: ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace; }
.small { color: #666; font-size: 0.9rem; }
</style>
</head>
<body>
  <h1>Index of <span class="code">${escapeHtml(reqPath)}</span></h1>
  <div class="small">Root: <span class="code">${escapeHtml(path.relative(process.cwd(), rootFsPath) || '.')}</span></div>
  <nav>${bc}</nav>
  <ul>
  ${items}
  </ul>
</body>
</html>`;
}

/**
 * PUBLIC_INTERFACE
 * startServer starts a minimal static file server.
 *
 * @param {object} [options]
 * @param {number} [options.port] Port to listen on. Defaults to process.env.PORT or 3000.
 * @param {string} [options.serveDir] Absolute path to directory to serve. Defaults to docs/ if present, else repo root.
 * @returns {http.Server} The created http server.
 */
function startServer(options = {}) {
  const port = Number.isFinite(options.port)
    ? options.port
    : Number.parseInt(process.env.PORT || '3000', 10);

  const repoRoot = path.resolve(__dirname, '..');
  const docsDir = path.join(repoRoot, 'docs');
  const serveDir =
    options.serveDir && path.isAbsolute(options.serveDir)
      ? options.serveDir
      : (fs.existsSync(docsDir) && fs.statSync(docsDir).isDirectory() ? docsDir : repoRoot);

  const server = http.createServer((req, res) => {
    try {
      const baseURL = `http://${req.headers.host || 'localhost'}`;
      const reqUrl = new URL(req.url || '/', baseURL);
      // Only use the pathname (ignore querystring)
      let reqPath = decodeURIComponent(reqUrl.pathname);

      // Normalize and ensure leading slash
      if (!reqPath.startsWith('/')) reqPath = `/${reqPath}`;

      // Construct filesystem path
      const fsPath = path.join(serveDir, reqPath);

      // Security: ensure requested path is inside serveDir
      const resolved = path.resolve(fsPath);
      if (!isPathInside(resolved, serveDir) && resolved !== serveDir) {
        res.writeHead(403, { 'Content-Type': 'text/plain; charset=utf-8' });
        res.end('403 Forbidden');
        return;
      }

      // If path points to a directory, try index.html; otherwise show listing
      let stat;
      try {
        stat = fs.statSync(resolved);
      } catch {
        stat = null;
      }

      if (stat && stat.isDirectory()) {
        const indexPath = path.join(resolved, 'index.html');
        if (fs.existsSync(indexPath) && fs.statSync(indexPath).isFile()) {
          res.writeHead(200, {
            'Content-Type': contentTypeFor(indexPath),
            'Cache-Control': 'no-store'
          });
          fs.createReadStream(indexPath).pipe(res);
          return;
        }
        // Directory listing
        const html = directoryListingHTML(resolved, reqPath.endsWith('/') ? reqPath : `${reqPath}/`, serveDir);
        res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-store' });
        res.end(html);
        return;
      }

      // Serve file if exists
      if (stat && stat.isFile()) {
        res.writeHead(200, {
          'Content-Type': contentTypeFor(resolved),
          'Cache-Control': 'no-store'
        });
        fs.createReadStream(resolved)
          .on('error', () => {
            res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
            res.end('500 Internal Server Error');
          })
          .pipe(res);
        return;
      }

      // Not found
      res.writeHead(404, { 'Content-Type': 'text/plain; charset=utf-8', 'Cache-Control': 'no-store' });
      res.end('404 Not Found');
    } catch (err) {
      // Unexpected error
      res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8', 'Cache-Control': 'no-store' });
      res.end('500 Internal Server Error');
    }
  });

  server.listen(port, '0.0.0.0', () => {
    console.log(`app-gateway2: serving directory: ${serveDir}`);
    console.log(`app-gateway2: server listening on http://0.0.0.0:${port}`);
  });

  return server;
}

module.exports = { startServer };

// Allow running directly via "node scripts/server.js"
if (require.main === module) {
  startServer();
}
