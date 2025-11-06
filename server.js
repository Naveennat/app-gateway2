/**
 * Minimal HTTP server for the app-gateway2 container.
 * - Serves documentation and any available HTML test reports
 * - Binds to 0.0.0.0 and respects PORT env variable
 * - Avoids external dependencies
 */

const http = require('http');
const fs = require('fs');
const path = require('path');
const url = require('url');

// Directories we can serve from
const BASE_DIR = __dirname;
const STATIC_MOUNTS = [
  { urlPrefix: '/docs', dir: path.join(BASE_DIR, 'docs') },
  { urlPrefix: '/reports', dir: path.join(BASE_DIR, 'Supporting_Files', 'Badger_Test_Results', 'HTML_Report') },
  { urlPrefix: '/build-ottservices', dir: path.join(BASE_DIR, 'build-ottservices') }
];

// Map of common content types
const CONTENT_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.htm': 'text/html; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.mjs': 'application/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.txt': 'text/plain; charset=utf-8',
  '.md': 'text/plain; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.ico': 'image/x-icon'
};

// PUBLIC_INTERFACE
function startServer({ host, port } = {}) {
  const serverHost = host || process.env.HOST || '0.0.0.0';
  const serverPort = Number(process.env.PORT || port || 3000);

  const server = http.createServer(async (req, res) => {
    try {
      const parsedUrl = url.parse(req.url || '/');
      const pathname = decodeURIComponent(parsedUrl.pathname || '/');

      // Health endpoint
      if (pathname === '/health' || pathname === '/healthz') {
        return sendText(res, 200, 'ok');
      }

      // Root index page
      if (pathname === '/' || pathname === '/index.html') {
        return sendHtml(res, 200, renderIndexPage());
      }

      // Serve README.md (as text)
      if (pathname === '/README.md') {
        const readmePath = path.join(BASE_DIR, 'README.md');
        if (await exists(readmePath)) {
          return streamFile(res, readmePath);
        }
        return sendText(res, 404, 'README.md not found');
      }

      // Try to serve from mounted static directories
      const staticCandidate = resolveStaticPath(pathname);
      if (staticCandidate) {
        return servePath(res, staticCandidate);
      }

      // As a fallback, attempt to serve files from the base directory
      const fallbackPath = path.join(BASE_DIR, pathname.replace(/^\//, ''));
      if (await safePathExists(fallbackPath, BASE_DIR)) {
        return servePath(res, fallbackPath);
      }

      // Not found
      return sendText(res, 404, 'Not Found');
    } catch (err) {
      console.error('Server error:', err);
      return sendText(res, 500, 'Internal Server Error');
    }
  });

  server.listen(serverPort, serverHost, () => {
    console.log(`[app-gateway2] Preview server listening on http://${serverHost}:${serverPort}`);
  });

  return server;
}

// Utilities

function contentTypeFor(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  return CONTENT_TYPES[ext] || 'application/octet-stream';
}

function sendText(res, status, text) {
  res.writeHead(status, { 'Content-Type': 'text/plain; charset=utf-8' });
  res.end(text);
}

function sendHtml(res, status, html) {
  res.writeHead(status, { 'Content-Type': 'text/html; charset=utf-8' });
  res.end(html);
}

async function exists(p) {
  try {
    await fs.promises.access(p, fs.constants.F_OK);
    return true;
  } catch {
    return false;
  }
}

async function safePathExists(targetPath, allowedRoot) {
  const normalized = path.normalize(targetPath);
  const normalizedRoot = path.normalize(allowedRoot);
  if (!normalized.startsWith(normalizedRoot)) {
    return false;
  }
  return exists(normalized);
}

function resolveStaticPath(requestPath) {
  for (const mount of STATIC_MOUNTS) {
    if (requestPath === mount.urlPrefix || requestPath.startsWith(mount.urlPrefix + '/')) {
      const rel = requestPath.slice(mount.urlPrefix.length);
      const candidate = path.join(mount.dir, rel.replace(/^\//, ''));
      // Ensure no path traversal outside mount dir
      const normalized = path.normalize(candidate);
      if (normalized.startsWith(path.normalize(mount.dir))) {
        return normalized;
      }
      return null;
    }
  }
  return null;
}

async function servePath(res, candidate) {
  try {
    const stat = await fs.promises.stat(candidate);
    if (stat.isDirectory()) {
      // If an index.html exists, serve it; otherwise render a simple directory listing
      const indexHtml = path.join(candidate, 'index.html');
      if (await exists(indexHtml)) {
        return streamFile(res, indexHtml);
      }
      const listing = await renderDirectoryListing(candidate);
      return sendHtml(res, 200, listing);
    } else {
      return streamFile(res, candidate);
    }
  } catch (err) {
    console.error('Error serving path:', candidate, err);
    return sendText(res, 404, 'Not Found');
  }
}

function streamFile(res, filePath) {
  const ct = contentTypeFor(filePath);
  res.writeHead(200, { 'Content-Type': ct });
  const stream = fs.createReadStream(filePath);
  stream.on('error', (err) => {
    console.error('Stream error:', err);
    if (!res.headersSent) {
      res.writeHead(500, { 'Content-Type': 'text/plain; charset=utf-8' });
    }
    res.end('File read error');
  });
  stream.pipe(res);
}

async function renderDirectoryListing(dirPath) {
  let items = [];
  try {
    items = await fs.promises.readdir(dirPath, { withFileTypes: true });
  } catch {
    // ignore
  }
  const relRoot = path.relative(BASE_DIR, dirPath) || '.';
  const links = items
    .map((ent) => {
      const name = ent.name + (ent.isDirectory() ? '/' : '');
      const href = path.join('/', relRoot, ent.name).replace(/\\/g, '/');
      return `<li><a href="${escapeHtml(href)}">${escapeHtml(name)}</a></li>`;
    })
    .join('\n');

  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>Index of /${escapeHtml(relRoot)}</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    body { font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif; margin: 2rem; line-height: 1.5; }
    .container { max-width: 960px; margin: 0 auto; }
    h1 { font-size: 1.25rem; }
    ul { list-style: none; padding-left: 0; }
    li { margin: 0.25rem 0; }
    a { text-decoration: none; color: #0b5fff; }
    a:hover { text-decoration: underline; }
  </style>
</head>
<body>
  <div class="container">
    <h1>Index of /${escapeHtml(relRoot)}</h1>
    <ul>
      ${links || '<li><em>No files</em></li>'}
    </ul>
    <p><a href="/">Back to home</a></p>
  </div>
</body>
</html>`;
}

function escapeHtml(str) {
  return String(str)
    .replaceAll('&', '&amp;')
    .replaceAll('<', '&lt;')
    .replaceAll('>', '&gt;')
    .replaceAll('"', '&quot;')
    .replaceAll("'", '&#39;');
}

function renderIndexPage() {
  const hasReadme = fs.existsSync(path.join(BASE_DIR, 'README.md'));
  const docsDir = path.join(BASE_DIR, 'docs');
  const hasDocs = fs.existsSync(docsDir);
  const reportDir = path.join(BASE_DIR, 'Supporting_Files', 'Badger_Test_Results', 'HTML_Report');
  const hasReports = fs.existsSync(reportDir);

  return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <title>app-gateway2 Preview</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    body { font-family: system-ui, -apple-system, Segoe UI, Roboto, sans-serif; margin: 2rem; line-height: 1.6; }
    .container { max-width: 960px; margin: 0 auto; }
    h1 { font-size: 1.5rem; margin-bottom: 0.5rem; }
    h2 { font-size: 1.1rem; margin-top: 1.5rem; }
    code { background: #f6f8fa; padding: 0.2rem 0.4rem; border-radius: 4px; }
    a { color: #0b5fff; text-decoration: none; }
    a:hover { text-decoration: underline; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 1rem; }
    .card { border: 1px solid #e5e7eb; border-radius: 8px; padding: 1rem; background: #fff; }
  </style>
</head>
<body>
  <div class="container">
    <h1>app-gateway2 Preview</h1>
    <p>This lightweight server exposes documentation and any generated HTML reports from the repository.</p>

    <div class="grid">
      <div class="card">
        <h2>Health</h2>
        <p><a href="/health">/health</a></p>
        <p>Server binds to <code>${escapeHtml(process.env.HOST || '0.0.0.0')}</code> on port <code>${escapeHtml(process.env.PORT || '3000')}</code>.</p>
      </div>

      <div class="card">
        <h2>README</h2>
        <p>${hasReadme ? '<a href="/README.md">View README.md</a>' : '<em>No README.md found</em>'}</p>
      </div>

      <div class="card">
        <h2>Docs</h2>
        <p>${hasDocs ? '<a href="/docs/">Browse /docs</a>' : '<em>No docs/ folder found</em>'}</p>
      </div>

      <div class="card">
        <h2>HTML Reports</h2>
        <p>${hasReports ? '<a href="/reports/">Browse HTML Report</a>' : '<em>No HTML reports found</em>'}</p>
      </div>
    </div>
  </div>
</body>
</html>`;
}

// Auto-start if executed directly
if (require.main === module) {
  startServer();
}

// Exported for potential reuse/testing
module.exports = { startServer };
