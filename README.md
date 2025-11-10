# app-gateway2 Minimal Web Gateway

This directory provides a minimal Express-based HTTP server for health checks and preview environment validation.

## Usage

- **Start in production mode:**  
  ```
  npm install
  npm start
  ```
  or  
  ```
  yarn install
  yarn start
  ```

- **Development mode (auto-reload):**  
  ```
  npm run dev
  ```
  or  
  ```
  yarn run dev
  ```

- **Health check:**  
  Open your browser at [http://localhost:3000/](http://localhost:3000/) (or on the port assigned in the PORT environment variable).

- If a `public/` or `dist/` directory exists, its contents (such as an `index.html`) will be served automatically at the root endpoint.

## Notes

- The actual app logic is not implemented in this minimal setup.  
- Do not change the port in config; always relies on the `PORT` environment variable.
- The server is suitable for preview/health check only.

