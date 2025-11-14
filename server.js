const express = require('express');
const path = require('path');
require('dotenv').config();

const app = express();
const PORT = process.env.PORT || 3000;

// Serve static files from the 'public' directory
app.use(express.static(path.join(__dirname, 'public')));

// Health endpoint
// PUBLIC_INTERFACE
app.get('/healthz', (req, res) => {
    /** Health check endpoint, returns 200 OK if service is running */
    res.status(200).send('OK');
});

// Catch-all (optional): if not found, fallback to index.html for SPA-like experience
app.get('*', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

app.listen(PORT, () => {
    console.log(`app-gateway2 is running at http://localhost:${PORT}`);
});
