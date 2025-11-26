# app-gateway2

Minimal Express server for preview environments.

How to run locally
- npm install
- HOST=0.0.0.0 PORT=3000 npm start
- Or: ./start.sh

Preview/Container start
- Procfile is configured as: web: ./start.sh
- Dockerfile provided with CMD ./start.sh and ENV HOST/PORT defaults.

Health check
- GET /health returns HTTP 200 with 'ok'.
