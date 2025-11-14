# app-gateway2

This is the app-gateway2 service.

## Getting Started

1. **Install Dependencies**

   ```
   npm install
   ```

2. **Set Environment Variables**

   Copy `.env.example` to `.env` and edit the `PORT` value as needed.
   ```
   cp .env.example .env
   ```

3. **Start the Service**

   ```
   npm start
   ```

   By default, the service will run on the port specified by `PORT` in your `.env` file (default: 3000).

### Express App Details

- The server entrypoint is `server.js` and can be started with:
  ```sh
  npm start
  ```
  This runs `node server.js` using the start script in `package.json`.
- The server listens on the `PORT` environment variable (defaults to 3000 if not set).

### Static Files

- If you add a file as `public/index.html`, it will be served at the root path `/`.
- Accessing `/` loads the static HTML if present, or returns a basic 'app-gateway2 running' message otherwise.

## Endpoints

- `/` - Health check, returns simple text.

## Requirements

- Node.js (v14 or higher recommended)
- [express](https://expressjs.com/)

## License

See [LICENSE](../LICENSE) for details.
