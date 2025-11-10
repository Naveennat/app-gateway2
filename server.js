const express = require('express');
const path = require('path');
const app = express();

const PORT = process.env.PORT || 3000;

// Try to serve static files if dist/ or public/ exists, else serve basic HTML
const staticDirs = ['dist', 'public'];
let staticDirServed = false;

for (const dir of staticDirs) {
    try {
        if (require('fs').existsSync(path.join(__dirname, dir))) {
            app.use(express.static(path.join(__dirname, dir)));
            staticDirServed = true;
            break;
        }
    } catch (e) {
        // continue
    }
}

// Liveness endpoint for container/preview health checks
// PUBLIC_INTERFACE
app.get('/health', (req, res) => {
    res.status(200).json({ status: 'ok' });
});

// Root endpoint - serves static index.html if available, else simple HTML
// PUBLIC_INTERFACE
app.get('/', (req, res) => {
    if (staticDirServed) {
        // Ordinary static fallback handled by Express if index.html in static dir
        res.sendFile('index.html', { root: path.join(__dirname, staticDirs.find(d => require('fs').existsSync(path.join(__dirname, d)))) }, err => {
            if (err) res.type('text/html').status(200).send('<h1>App Gateway2</h1><p>Static build not found. Server running.</p>');
        });
    } else {
        res.type('text/html').status(200).send('<h1>App Gateway2</h1><p>Server is running.</p>');
    }
});

app.listen(PORT, () => {
    console.log(`app-gateway2 Express server listening on port ${PORT}`);
});

module.exports = app;
