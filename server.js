const path = require('path');
const express = require('express');

const app = express();
const PORT = process.env.PORT || 3000;

// Serve static files from 'public' if it exists, otherwise from the current directory
try {
  app.use(express.static(path.join(__dirname, 'public')));
} catch {
  app.use(express.static(__dirname));
}

// Fallback route for SPA or general catch-all (optional)
app.get('*', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'), err => {
    if (err) {
      // If not in public, try root
      res.sendFile(path.join(__dirname, 'index.html'));
    }
  });
});

app.listen(PORT, () => {
  console.log(`Server is listening on port ${PORT}`);
});
