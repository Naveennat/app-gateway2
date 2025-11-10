# syntax=docker/dockerfile:1
# Minimal container to run the app-gateway2 preview server.

FROM node:18-alpine AS runtime

# Ensure deterministic defaults and a non-root-friendly port
ENV NODE_ENV=production \
    HOST=0.0.0.0 \
    PORT=3000

WORKDIR /usr/src/app

# Copy dependency manifest first for better layer caching and install deps
COPY package*.json ./
RUN npm install --omit=dev

# Copy only what's needed to run the preview server
COPY server.js ./
COPY index.js ./
COPY README.md ./

# Container documentation; the platform may use PORT env for mapping
EXPOSE 3000

# Lightweight healthcheck without adding curl/wget packages
# It uses Node to probe /health; exits 0 on 200, otherwise 1
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD node -e "require('http').get({hostname:'127.0.0.1',port:process.env.PORT||3000,path:'/health'},res=>{process.exit(res.statusCode===200?0:1)}).on('error',()=>process.exit(1))"

# Start the preview server
CMD ["node", "index.js"]
