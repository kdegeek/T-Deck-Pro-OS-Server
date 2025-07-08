# T-Deck-Pro Server Infrastructure Deployment Guide

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Environment Setup](#environment-setup)
3. [Configuration](#configuration)
4. [Local Development Deployment](#local-development-deployment)
5. [Staging Deployment](#staging-deployment)
6. [Production Deployment](#production-deployment)
7. [SSL Certificate Setup](#ssl-certificate-setup)
8. [Tailscale VPN Configuration](#tailscale-vpn-configuration)
9. [Monitoring and Logging](#monitoring-and-logging)
10. [Backup and Recovery](#backup-and-recovery)
11. [Scaling and Performance](#scaling-and-performance)
12. [Troubleshooting](#troubleshooting)

## Prerequisites

### System Requirements

**Minimum Requirements (Development)**:
- CPU: 2 cores
- RAM: 4GB
- Storage: 20GB SSD
- Network: 10 Mbps

**Recommended Requirements (Production)**:
- CPU: 4+ cores
- RAM: 8GB+
- Storage: 100GB+ SSD
- Network: 100 Mbps+

### Software Dependencies

```bash
# Required software
- Docker 24.0+
- Docker Compose 2.0+
- Git
- curl/wget
- openssl

# Optional but recommended
- htop
- iotop
- netstat
- tcpdump
```

### Network Requirements

```bash
# Required ports (internal)
- 80/tcp    # HTTP (redirects to HTTPS)
- 443/tcp   # HTTPS API endpoints
- 5432/tcp  # PostgreSQL (internal)
- 6379/tcp  # Redis (internal)
- 8086/tcp  # InfluxDB (internal)

# Optional ports (monitoring)
- 3001/tcp  # Grafana dashboard
- 5601/tcp  # Kibana logs
- 9090/tcp  # Prometheus metrics

# Tailscale VPN
- 41641/udp # Tailscale default port
```

## Environment Setup

### 1. Server Preparation

```bash
# Update system
sudo apt update && sudo apt upgrade -y

# Install Docker
curl -fsSL https://get.docker.com -o get-docker.sh
sudo sh get-docker.sh
sudo usermod -aG docker $USER

# Install Docker Compose
sudo curl -L "https://github.com/docker/compose/releases/latest/download/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose

# Verify installation
docker --version
docker-compose --version
```

### 2. Repository Setup

```bash
# Clone repository
git clone <repository-url>
cd server-infrastructure

# Create required directories
mkdir -p {data,logs,config,ssl}
mkdir -p data/{ota-updates,backups}
mkdir -p logs/{api,ota,telemetry,bridge,postgres,nginx}
mkdir -p config/{nginx,postgres,redis,grafana,prometheus}
```

### 3. Environment Configuration

```bash
# Copy environment template
cp config/example.env .env

# Generate secure secrets
./scripts/generate-secrets.sh

# Edit configuration
nano .env
```

## Configuration

### Environment Variables (.env)

```bash
# Basic Configuration
NODE_ENV=production
DOMAIN_NAME=api.tdeckpro.local
API_PORT=3000

# Database Configuration
DB_PASSWORD=<secure-random-password>
POSTGRES_DB=tdeckpro
POSTGRES_USER=tdeckpro

# Redis Configuration
REDIS_PASSWORD=<secure-random-password>

# Security Keys
JWT_SECRET=<64-character-random-string>
ENCRYPTION_KEY=<32-character-random-string>
OTA_SIGNING_KEY=<path-to-private-key>

# Tailscale Configuration
TAILSCALE_AUTH_KEY=tskey-auth-<your-auth-key>

# InfluxDB Configuration
INFLUXDB_USERNAME=admin
INFLUXDB_PASSWORD=<secure-random-password>
INFLUXDB_ORG=tdeckpro
INFLUXDB_TOKEN=<influxdb-admin-token>

# Grafana Configuration
GRAFANA_PASSWORD=<secure-random-password>

# External API Keys
WEATHER_API_KEY=<your-weather-api-key>
MAPS_API_KEY=<your-maps-api-key>
EMERGENCY_API_KEY=<your-emergency-api-key>

# MQTT Configuration (optional)
MQTT_BROKER_URL=mqtt://broker.hivemq.com:1883
MQTT_USERNAME=<mqtt-username>
MQTT_PASSWORD=<mqtt-password>

# Monitoring
PROMETHEUS_RETENTION=200h
LOG_LEVEL=info
```

### SSL Certificate Configuration

```bash
# Option 1: Let's Encrypt (Recommended for production)
./scripts/setup-letsencrypt.sh api.tdeckpro.local

# Option 2: Self-signed certificates (Development)
./scripts/generate-self-signed-certs.sh

# Option 3: Custom certificates
# Copy your certificates to ssl/ directory:
# ssl/cert.pem
# ssl/key.pem
# ssl/ca.pem (if using CA)
```

### Nginx Configuration

Create `config/nginx/nginx.conf`:

```nginx
user nginx;
worker_processes auto;
error_log /var/log/nginx/error.log warn;
pid /var/run/nginx.pid;

events {
    worker_connections 1024;
    use epoll;
    multi_accept on;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    # Logging
    log_format main '$remote_addr - $remote_user [$time_local] "$request" '
                    '$status $body_bytes_sent "$http_referer" '
                    '"$http_user_agent" "$http_x_forwarded_for"';
    access_log /var/log/nginx/access.log main;

    # Performance
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;
    types_hash_max_size 2048;
    client_max_body_size 50M;

    # Gzip compression
    gzip on;
    gzip_vary on;
    gzip_min_length 1024;
    gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;

    # Security headers
    add_header X-Frame-Options DENY;
    add_header X-Content-Type-Options nosniff;
    add_header X-XSS-Protection "1; mode=block";
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;

    # Rate limiting
    limit_req_zone $binary_remote_addr zone=api:10m rate=10r/s;
    limit_req_zone $binary_remote_addr zone=auth:10m rate=5r/m;

    # Upstream servers
    upstream api_backend {
        least_conn;
        server api-server:3000 max_fails=3 fail_timeout=30s;
    }

    upstream ota_backend {
        least_conn;
        server ota-service:8000 max_fails=3 fail_timeout=30s;
    }

    upstream telemetry_backend {
        least_conn;
        server telemetry-collector:8001 max_fails=3 fail_timeout=30s;
    }

    upstream bridge_backend {
        least_conn;
        server meshtastic-bridge:8002 max_fails=3 fail_timeout=30s;
    }

    # HTTP to HTTPS redirect
    server {
        listen 80;
        server_name api.tdeckpro.local;
        return 301 https://$server_name$request_uri;
    }

    # Main HTTPS server
    server {
        listen 443 ssl http2;
        server_name api.tdeckpro.local;

        # SSL configuration
        ssl_certificate /etc/nginx/ssl/cert.pem;
        ssl_certificate_key /etc/nginx/ssl/key.pem;
        ssl_protocols TLSv1.2 TLSv1.3;
        ssl_ciphers ECDHE-RSA-AES256-GCM-SHA512:DHE-RSA-AES256-GCM-SHA512:ECDHE-RSA-AES256-GCM-SHA384:DHE-RSA-AES256-GCM-SHA384;
        ssl_prefer_server_ciphers off;
        ssl_session_cache shared:SSL:10m;
        ssl_session_timeout 10m;

        # API endpoints
        location /api/v1/ {
            limit_req zone=api burst=20 nodelay;
            proxy_pass http://api_backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            proxy_connect_timeout 30s;
            proxy_send_timeout 30s;
            proxy_read_timeout 30s;
        }

        # Authentication endpoints (stricter rate limiting)
        location /api/v1/auth/ {
            limit_req zone=auth burst=5 nodelay;
            proxy_pass http://api_backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
        }

        # OTA update endpoints
        location /api/v1/ota/ {
            client_max_body_size 100M;
            proxy_pass http://ota_backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            proxy_connect_timeout 60s;
            proxy_send_timeout 300s;
            proxy_read_timeout 300s;
        }

        # Telemetry endpoints
        location /api/v1/telemetry/ {
            proxy_pass http://telemetry_backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
        }

        # Mesh bridge endpoints
        location /api/v1/mesh/ {
            proxy_pass http://bridge_backend;
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
        }

        # WebSocket endpoints
        location /ws {
            proxy_pass http://api_backend;
            proxy_http_version 1.1;
            proxy_set_header Upgrade $http_upgrade;
            proxy_set_header Connection "upgrade";
            proxy_set_header Host $host;
            proxy_set_header X-Real-IP $remote_addr;
            proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
            proxy_set_header X-Forwarded-Proto $scheme;
            proxy_connect_timeout 7d;
            proxy_send_timeout 7d;
            proxy_read_timeout 7d;
        }

        # Health check endpoint
        location /health {
            access_log off;
            return 200 "healthy\n";
            add_header Content-Type text/plain;
        }

        # Static files (if any)
        location /static/ {
            alias /var/www/static/;
            expires 1y;
            add_header Cache-Control "public, immutable";
        }
    }
}
```

## Local Development Deployment

### Quick Start

```bash
# 1. Clone and setup
git clone <repository-url>
cd server-infrastructure
cp config/example.env .env

# 2. Generate development secrets
./scripts/generate-dev-secrets.sh

# 3. Start development stack
docker-compose -f docker-compose.dev.yml up -d

# 4. Initialize database
./scripts/init-database.sh

# 5. Verify deployment
curl -k https://localhost/api/v1/system/status
```

### Development Configuration

Create `docker-compose.dev.yml`:

```yaml
version: '3.8'

services:
  # Override production settings for development
  api-server:
    build:
      context: .
      dockerfile: Dockerfile.api
      target: development
    environment:
      - NODE_ENV=development
      - LOG_LEVEL=debug
    volumes:
      - ./src/api-server:/app:ro
      - /app/node_modules
    ports:
      - "3000:3000"

  postgres:
    ports:
      - "5432:5432"

  redis:
    ports:
      - "6379:6379"

  # Remove production-only services
  tailscale:
    profiles: ["production"]

  elasticsearch:
    profiles: ["production"]

  logstash:
    profiles: ["production"]

  kibana:
    profiles: ["production"]
```

## Production Deployment

### Pre-deployment Checklist

```bash
# 1. System preparation
□ Server meets minimum requirements
□ Docker and Docker Compose installed
□ Firewall configured
□ SSL certificates obtained
□ Tailscale account setup
□ Domain DNS configured

# 2. Security preparation
□ Strong passwords generated
□ SSH keys configured
□ Non-root user created
□ Fail2ban installed (optional)
□ Backup strategy planned

# 3. Configuration preparation
□ Environment variables configured
□ SSL certificates in place
□ Nginx configuration reviewed
□ Database initialization scripts ready
□ Monitoring dashboards configured
```

### Deployment Steps

```bash
# 1. Final system preparation
sudo ufw enable
sudo ufw allow ssh
sudo ufw allow 80/tcp
sudo ufw allow 443/tcp

# 2. Deploy application
docker-compose up -d

# 3. Initialize database
./scripts/init-database.sh

# 4. Setup monitoring
./scripts/setup-monitoring.sh

# 5. Configure backups
./scripts/setup-backups.sh

# 6. Verify deployment
./scripts/health-check.sh
```

### Post-deployment Verification

```bash
# 1. Service health checks
docker-compose ps
docker-compose logs --tail=50

# 2. API endpoint tests
curl -k https://api.tdeckpro.local/api/v1/system/status
curl -k https://api.tdeckpro.local/health

# 3. Database connectivity
docker-compose exec postgres psql -U tdeckpro -d tdeckpro -c "SELECT version();"

# 4. Cache connectivity
docker-compose exec redis redis-cli ping

# 5. Tailscale connectivity
docker-compose exec tailscale tailscale status

# 6. Log aggregation
curl http://localhost:5601/api/status

# 7. Monitoring
curl http://localhost:3001/api/health
curl http://localhost:9090/api/v1/query?query=up
```

## SSL Certificate Setup

### Let's Encrypt (Production)

```bash
# Install certbot
sudo apt install certbot

# Generate certificate
sudo certbot certonly --standalone -d api.tdeckpro.local

# Copy certificates
sudo cp /etc/letsencrypt/live/api.tdeckpro.local/fullchain.pem ssl/cert.pem
sudo cp /etc/letsencrypt/live/api.tdeckpro.local/privkey.pem ssl/key.pem
sudo chown $USER:$USER ssl/*.pem

# Setup auto-renewal
echo "0 12 * * * /usr/bin/certbot renew --quiet && docker-compose restart nginx" | sudo crontab -
```

### Self-signed Certificates (Development)

```bash
# Generate self-signed certificate
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout ssl/key.pem \
    -out ssl/cert.pem \
    -subj "/C=US/ST=State/L=City/O=Organization/CN=api.tdeckpro.local"

# Set permissions
chmod 600 ssl/key.pem
chmod 644 ssl/cert.pem
```

## Tailscale VPN Configuration

### Setup Tailscale

```bash
# 1. Create Tailscale account at https://tailscale.com
# 2. Generate auth key in admin console
# 3. Configure ACLs for device access

# Example ACL configuration:
{
  "acls": [
    {
      "action": "accept",
      "src": ["group:t-deck-devices"],
      "dst": ["group:servers:*"]
    },
    {
      "action": "accept",
      "src": ["group:admins"],
      "dst": ["*:*"]
    }
  ],
  "groups": {
    "group:t-deck-devices": ["tag:t-deck"],
    "group:servers": ["tag:server"],
    "group:admins": ["user@example.com"]
  },
  "tagOwners": {
    "tag:t-deck": ["user@example.com"],
    "tag:server": ["user@example.com"]
  }
}
```

### Verify Tailscale Connection

```bash
# Check Tailscale status
docker-compose exec tailscale tailscale status

# Test connectivity from T-Deck device
# Device should be able to reach: https://<tailscale-ip>/api/v1/system/status
```

## Monitoring and Logging

### Grafana Dashboards

Access Grafana at `http://localhost:3001`:

1. **System Overview Dashboard**
   - CPU, Memory, Disk usage
   - Network traffic
   - Container health

2. **Application Metrics Dashboard**
   - API request rates
   - Response times
   - Error rates
   - Database performance

3. **Device Telemetry Dashboard**
   - Device status overview
   - Battery levels
   - Signal strengths
   - Geographic distribution

4. **OTA Update Dashboard**
   - Update campaign progress
   - Success/failure rates
   - Download statistics

### Log Analysis with ELK

Access Kibana at `http://localhost:5601`:

1. **Index Patterns**
   - `tdeckpro-api-*`
   - `tdeckpro-ota-*`
   - `tdeckpro-telemetry-*`
   - `tdeckpro-bridge-*`

2. **Common Searches**
   - Error logs: `level:error`
   - API errors: `service:api AND status:>=400`
   - Device authentication: `component:auth`
   - OTA updates: `service:ota AND event:update`

### Alerting Rules

Configure Prometheus alerts in `config/prometheus/alerts.yml`:

```yaml
groups:
  - name: tdeckpro-alerts
    rules:
      - alert: HighErrorRate
        expr: rate(http_requests_total{status=~"5.."}[5m]) > 0.1
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High error rate detected"

      - alert: DatabaseDown
        expr: up{job="postgres"} == 0
        for: 1m
        labels:
          severity: critical
        annotations:
          summary: "Database is down"

      - alert: HighMemoryUsage
        expr: (node_memory_MemTotal_bytes - node_memory_MemAvailable_bytes) / node_memory_MemTotal_bytes > 0.9
        for: 5m
        labels:
          severity: warning
        annotations:
          summary: "High memory usage"
```

## Backup and Recovery

### Automated Backup Script

Create `scripts/backup.sh`:

```bash
#!/bin/bash

BACKUP_DIR="/data/backups"
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_NAME="tdeckpro_backup_$DATE"

# Create backup directory
mkdir -p "$BACKUP_DIR/$BACKUP_NAME"

# Database backup
docker-compose exec -T postgres pg_dump -U tdeckpro tdeckpro > "$BACKUP_DIR/$BACKUP_NAME/database.sql"

# Redis backup
docker-compose exec -T redis redis-cli --rdb - > "$BACKUP_DIR/$BACKUP_NAME/redis.rdb"

# InfluxDB backup
docker-compose exec -T influxdb influx backup /tmp/backup
docker cp $(docker-compose ps -q influxdb):/tmp/backup "$BACKUP_DIR/$BACKUP_NAME/influxdb"

# Configuration backup
cp -r config "$BACKUP_DIR/$BACKUP_NAME/"
cp .env "$BACKUP_DIR/$BACKUP_NAME/"

# OTA files backup
cp -r data/ota-updates "$BACKUP_DIR/$BACKUP_NAME/"

# Compress backup
tar -czf "$BACKUP_DIR/$BACKUP_NAME.tar.gz" -C "$BACKUP_DIR" "$BACKUP_NAME"
rm -rf "$BACKUP_DIR/$BACKUP_NAME"

# Cleanup old backups (keep last 7 days)
find "$BACKUP_DIR" -name "tdeckpro_backup_*.tar.gz" -mtime +7 -delete

echo "Backup completed: $BACKUP_DIR/$BACKUP_NAME.tar.gz"
```

### Recovery Procedures

```bash
# 1. Stop services
docker-compose down

# 2. Extract backup
tar -xzf /data/backups/tdeckpro_backup_YYYYMMDD_HHMMSS.tar.gz -C /tmp

# 3. Restore database
docker-compose up -d postgres
sleep 10
docker-compose exec -T postgres psql -U tdeckpro -d tdeckpro < /tmp/tdeckpro_backup_*/database.sql

# 4. Restore Redis
docker-compose up -d redis
sleep 5
docker cp /tmp/tdeckpro_backup_*/redis.rdb $(docker-compose ps -q redis):/data/dump.rdb
docker-compose restart redis

# 5. Restore InfluxDB
docker-compose up -d influxdb
sleep 10
docker cp /tmp/tdeckpro_backup_*/influxdb $(docker-compose ps -q influxdb):/tmp/restore
docker-compose exec influxdb influx restore /tmp/restore

# 6. Restore configuration
cp -r /tmp/tdeckpro_backup_*/config/* config/
cp /tmp/tdeckpro_backup_*/.env .

# 7. Restore OTA files
cp -r /tmp/tdeckpro_backup_*/ota-updates/* data/ota-updates/

# 8. Start all services
docker-compose up -d

# 9. Verify recovery
./scripts/health-check.sh
```

## Scaling and Performance

### Horizontal Scaling

```yaml
# docker-compose.scale.yml
version: '3.8'

services:
  api-server:
    deploy:
      replicas: 3
    
  ota-service:
    deploy:
      replicas: 2
      
  telemetry-collector:
    deploy:
      replicas: 2

  # Load balancer configuration
  nginx:
    volumes:
      - ./nginx/nginx-scaled.conf:/etc/nginx/nginx.conf:ro
```

### Performance Tuning

```bash
# PostgreSQL tuning
echo "shared_preload_libraries = 'pg_stat_statements'" >> config/postgres/postgresql.conf
echo "max_connections = 200" >> config/postgres/postgresql.conf
echo "shared_buffers = 256MB" >> config/postgres/postgresql.conf
echo "effective_cache_size = 1GB" >> config/postgres/postgresql.conf

# Redis tuning
echo "maxmemory 512mb" >> config/redis/redis.conf
echo "maxmemory-policy allkeys-lru" >> config/redis/redis.conf

# Nginx tuning
echo "worker_processes auto;" >> config/nginx/nginx.conf
echo "worker_connections 2048;" >> config/nginx/nginx.conf
```

### Database Optimization

```sql
-- Create indexes for common queries
CREATE INDEX CONCURRENTLY idx_devices_status ON devices(status);
CREATE INDEX CONCURRENTLY idx_devices_last_seen ON devices(last_seen);
CREATE INDEX CONCURRENTLY idx_telemetry_device_time ON telemetry_data(device_id, timestamp DESC);
CREATE INDEX CONCURRENTLY idx_ota_updates_version ON ota_updates(version);

-- Partition telemetry table by time
CREATE TABLE telemetry_data_y2024m01 PARTITION OF telemetry_data
FOR VALUES FROM ('2024-01-01') TO ('2024-02-01');
```

## Troubleshooting

### Common Issues

**1. Container Won't Start**
```bash
# Check logs
docker-compose logs <service-name>

# Check resource usage
docker stats

# Check disk space
df -h
```

**2. Database Connection Issues**
```bash
# Test database connectivity
docker-compose exec postgres psql -U tdeckpro -d tdeckpro -c "SELECT 1;"

# Check database logs
docker-compose logs postgres

# Reset database
docker-compose down
docker volume rm server-infrastructure_postgres_data
docker-compose up -d postgres
./scripts/init-database.sh
```

**3. SSL Certificate Issues**
```bash
# Verify certificate
openssl x509 -in ssl/cert.pem -text -noout

# Test SSL connection
openssl s_client -connect api.tdeckpro.local:443

# Regenerate self-signed certificate
./scripts/generate-self-signed-certs.sh
```

**4. Tailscale Connectivity Issues**
```bash
# Check Tailscale status
docker-compose exec tailscale tailscale status

# Restart Tailscale
docker-compose restart tailscale

# Check Tailscale logs
docker-compose logs tailscale
```

**5. High Memory Usage**
```bash
# Check container memory usage
docker stats --no-stream

# Restart memory-intensive services
docker-compose restart elasticsearch logstash

# Adjust memory limits in docker-compose.yml
```

### Health Check Script

Create `scripts/health-check.sh`:

```bash
#!/bin/bash

echo "=== T-Deck-Pro Server Infrastructure Health Check ==="

# Check Docker services
echo "Checking Docker services..."
docker-compose ps

# Check API endpoints
echo "Checking API endpoints..."
curl -f -k https://localhost/api/v1/system/status || echo "API health check failed"

# Check database
echo "Checking database..."
docker-compose exec -T postgres pg_isready -U tdeckpro || echo "Database check failed"

# Check Redis
echo "Checking Redis..."
docker-compose exec -T redis redis-cli ping || echo "Redis check failed"

# Check disk space
echo "Checking disk space..."
df -h

# Check memory usage
echo "Checking memory usage..."
free -h

# Check Tailscale
echo "Checking Tailscale..."
docker-compose exec -T tailscale tailscale status || echo "Tailscale check failed"

echo "=== Health check completed ==="
```

### Log Analysis Commands

```bash
# View recent API logs
docker-compose logs --tail=100 api-server

# Search for errors
docker-compose logs | grep -i error

# Monitor logs in real-time
docker-compose logs -f

# Check specific service logs
docker-compose logs ota-service | grep -i "update"

# Export logs for analysis
docker-compose logs --since="2024-01-01T00:00:00Z" > logs/export.log
```

This deployment guide provides comprehensive instructions for setting up the T-Deck-Pro server infrastructure in development, staging, and production environments. Follow the appropriate sections based on your deployment target and requirements.