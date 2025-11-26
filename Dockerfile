# syntax=docker/dockerfile:1

# Simple production Dockerfile for app-gateway2
ARG NODE_VERSION=20-alpine
FROM node:${NODE_VERSION}

# Create non-root user already present in the node image
WORKDIR /app

# Ensure production environment
ENV NODE_ENV=production
ENV HOST=0.0.0.0
ENV PORT=3000

# Install dependencies first (leverage Docker layer cache)
COPY package*.json ./
RUN npm install --omit=dev --no-audit --no-fund

# Copy application source
COPY . .

# Ensure start script is executable
RUN chmod +x ./start.sh

# Expose default port (platform will pass PORT env)
EXPOSE 3000

# Start the app via the wrapper which respects HOST/PORT
CMD ["sh", "-c", "./start.sh"]
