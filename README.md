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

## Endpoints

- `/` - Health check, returns simple text.

## Requirements

- Node.js (v14 or higher recommended)
- [express](https://expressjs.com/)

## License

See [LICENSE](../LICENSE) for details.
