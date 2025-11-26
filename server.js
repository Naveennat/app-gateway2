const path = require('path');
const express = require('express');

const app = express();
const PORT = process.env.PORT || 3000;
const HOST = process.env.HOST || '0.0.0.0';

// Basic hardening: hide Express signature
app.disable('x-powered-by');

// Serve static files from 'public' if it exists, otherwise from the current directory
try {
  app.use(express.static(path.join(__dirname, 'public')));
} catch {
  app.use(express.static(__dirname));
}

// PUBLIC_INTERFACE
// Health check endpoint: returns { status: "ok" } with HTTP 200.
app.get('/health', (req, res) => {
  res.status(200).json({ status: 'ok' });
});

// PUBLIC_INTERFACE
// Root/fallback route for SPA or general catch-all.
// Tries to serve public/index.html or ./index.html; if neither exists, returns a simple text.
app.get('*', (req, res) => {
  const publicIndex = path.join(__dirname, 'public', 'index.html');
  const rootIndex = path.join(__dirname, 'index.html');

  res.sendFile(publicIndex, (err) => {
    if (err) {
      res.sendFile(rootIndex, (err2) => {
        if (err2) {
          res.status(200).send('app-gateway2 running');
        }
      });
    }
  });
});

app.listen(PORT, HOST, () => {
  console.log(`Server is listening on http://${HOST}:${PORT}`);
});
