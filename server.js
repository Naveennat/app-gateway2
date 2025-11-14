const express = require('express');
const path = require('path');

// PUBLIC_INTERFACE
// Entry point for app-gateway2 server.
// Starts an HTTP server listening on process.env.PORT
const app = express();

const PORT = process.env.PORT || 3000;

// Serve static files if public/index.html exists
const publicDir = path.join(__dirname, 'public');
app.use(express.static(publicDir));

// Example root endpoint
app.get('/', (req, res) => {
  // Serve index.html if it exists, else generic message
  const indexPath = path.join(publicDir, 'index.html');
  res.sendFile(indexPath, err => {
    if (err) res.status(200).send('app-gateway2 running');
  });
});

// Start server
app.listen(PORT, () => {
  console.log(`app-gateway2 listening on port ${PORT}`);
});
