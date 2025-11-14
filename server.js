const express = require('express');

const app = express();

// Simple health check endpoint
app.get('/', (req, res) => {
  res.send('App-Gateway2: Service Running');
});

// Use process.env.PORT or default to 3000 if not set
const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`App-Gateway2 server listening on port ${PORT}`);
});
