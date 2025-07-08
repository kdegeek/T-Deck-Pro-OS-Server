# T-Deck-Pro Server Infrastructure Development Manual

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Development Environment Setup](#development-environment-setup)
3. [Service Components](#service-components)
4. [API Development](#api-development)
5. [Database Design](#database-design)
6. [Security Implementation](#security-implementation)
7. [Integration with T-Deck-Pro OS](#integration-with-t-deck-pro-os)
8. [Testing Strategy](#testing-strategy)
9. [Deployment and DevOps](#deployment-and-devops)
10. [Monitoring and Observability](#monitoring-and-observability)
11. [Performance Optimization](#performance-optimization)
12. [Troubleshooting Guide](#troubleshooting-guide)

## Architecture Overview

### System Architecture

The T-Deck-Pro Server Infrastructure follows a microservices architecture designed for scalability, maintainability, and security. All services operate within a Tailscale VPN mesh network, ensuring secure communication between T-Deck-Pro devices and server components.

```
┌─────────────────────────────────────────────────────────────┐
│                    Tailscale VPN Mesh                      │
│  ┌─────────────────┐    ┌─────────────────────────────────┐ │
│  │   T-Deck-Pro    │    │        Server Infrastructure    │ │
│  │    Devices      │◄──►│                                 │ │
│  │                 │    │  ┌─────────────────────────────┐ │ │
│  │ • Device A      │    │  │      Load Balancer          │ │ │
│  │ • Device B      │    │  │        (Nginx)              │ │ │
│  │ • Device C      │    │  └─────────────┬───────────────┘ │ │
│  └─────────────────┘    │                │                 │ │
│                         │  ┌─────────────▼───────────────┐ │ │
│                         │  │       API Gateway           │ │ │
│                         │  │   (Authentication/Routing)  │ │ │
│                         │  └─────────────┬───────────────┘ │ │
│                         │                │                 │ │
│                         │  ┌─────────────▼───────────────┐ │ │
│                         │  │      Microservices          │ │ │
│                         │  │                             │ │ │
│                         │  │ ┌─────────┐ ┌─────────────┐ │ │ │
│                         │  │ │   API   │ │ OTA Update  │ │ │ │
│                         │  │ │ Server  │ │   Service   │ │ │ │
│                         │  │ └─────────┘ └─────────────┘ │ │ │
│                         │  │                             │ │ │
│                         │  │ ┌─────────┐ ┌─────────────┐ │ │ │
│                         │  │ │Meshtastic│ │ Telemetry  │ │ │ │
│                         │  │ │ Bridge  │ │ Collector   │ │ │ │
│                         │  │ └─────────┘ └─────────────┘ │ │ │
│                         │  └─────────────────────────────┘ │ │
│                         │                                 │ │
│                         │  ┌─────────────────────────────┐ │ │
│                         │  │      Data Layer             │ │ │
│                         │  │                             │ │ │
│                         │  │ ┌─────────┐ ┌─────────────┐ │ │ │
│                         │  │ │PostgreSQL│ │    Redis    │ │ │ │
│                         │  │ │Database │ │    Cache    │ │ │ │
│                         │  │ └─────────┘ └─────────────┘ │ │ │
│                         │  └─────────────────────────────┘ │ │
│                         └─────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

### Core Principles

1. **Security First**: All communication secured via Tailscale VPN
2. **Microservices**: Loosely coupled, independently deployable services
3. **API-Driven**: RESTful APIs with OpenAPI specifications
4. **Event-Driven**: Asynchronous communication via message queues
5. **Observability**: Comprehensive logging, metrics, and tracing
6. **Scalability**: Horizontal scaling with container orchestration
7. **Reliability**: High availability with graceful degradation

### Technology Stack

- **Runtime**: Node.js 18+ / Python 3.11+
- **API Framework**: Express.js / FastAPI
- **Database**: PostgreSQL 15+
- **Cache**: Redis 7+
- **Message Queue**: Redis Pub/Sub / RabbitMQ
- **Containerization**: Docker & Docker Compose
- **Reverse Proxy**: Nginx
- **VPN**: Tailscale
- **Monitoring**: Prometheus, Grafana, ELK Stack

## Development Environment Setup

### Prerequisites

```bash
# Required software
- Docker 24.0+
- Docker Compose 2.0+
- Node.js 18+ (for local development)
- Python 3.11+ (for local development)
- Git
- Tailscale CLI
```

### Initial Setup

1. **Clone Repository**
```bash
git clone <repository-url>
cd server-infrastructure
```

2. **Environment Configuration**
```bash
# Copy environment template
cp config/example.env .env

# Edit configuration
nano .env
```

3. **Tailscale Setup**
```bash
# Install Tailscale
curl -fsSL https://tailscale.com/install.sh | sh

# Authenticate
sudo tailscale up --auth-key=<your-auth-key>
```

4. **Development Dependencies**
```bash
# Install Node.js dependencies
npm install

# Install Python dependencies
pip install -r requirements.txt

# Install development tools
npm install -g nodemon jest
pip install pytest black flake8
```

### Local Development Environment

```bash
# Start development stack
docker-compose -f docker-compose.dev.yml up -d

# Start API server in development mode
npm run dev

# Run tests
npm test
pytest
```

### Environment Variables

```bash
# .env file configuration
NODE_ENV=development
PORT=3000
DATABASE_URL=postgresql://user:pass@localhost:5432/tdeckpro
REDIS_URL=redis://localhost:6379
TAILSCALE_AUTH_KEY=tskey-auth-xxxxx
JWT_SECRET=your-jwt-secret-here
ENCRYPTION_KEY=your-encryption-key-here

# External API Keys
WEATHER_API_KEY=your-weather-api-key
MAPS_API_KEY=your-maps-api-key
EMERGENCY_API_KEY=your-emergency-api-key

# Service Configuration
OTA_STORAGE_PATH=/data/ota-updates
TELEMETRY_RETENTION_DAYS=90
MAX_DEVICES_PER_USER=10
```

## Service Components

### 1. API Server

**Purpose**: Central API gateway for all T-Deck-Pro device communication

**Key Features**:
- Device registration and authentication
- User management and authorization
- Application management
- Configuration synchronization
- Real-time WebSocket connections

**Technology**: Node.js + Express.js + TypeScript

**Directory Structure**:
```
src/api-server/
├── controllers/          # Request handlers
├── middleware/          # Authentication, validation, etc.
├── models/             # Database models
├── routes/             # API route definitions
├── services/           # Business logic
├── utils/              # Helper functions
├── websocket/          # WebSocket handlers
└── app.ts              # Application entry point
```

**Key Endpoints**:
```typescript
// Device Management
POST   /api/v1/devices/register
GET    /api/v1/devices/:deviceId
PUT    /api/v1/devices/:deviceId/config
DELETE /api/v1/devices/:deviceId

// Application Management
GET    /api/v1/apps
POST   /api/v1/apps/:appId/install
DELETE /api/v1/apps/:appId/uninstall
GET    /api/v1/apps/:appId/config

// User Management
POST   /api/v1/auth/login
POST   /api/v1/auth/register
GET    /api/v1/users/profile
PUT    /api/v1/users/profile
```

### 2. OTA Update Service

**Purpose**: Manages firmware and application updates for T-Deck-Pro devices

**Key Features**:
- Version management and rollback
- Staged rollouts and A/B testing
- Update scheduling and automation
- Integrity verification and signing
- Bandwidth optimization

**Technology**: Python + FastAPI + Celery

**Directory Structure**:
```
src/ota-service/
├── api/                # FastAPI application
├── workers/            # Celery background tasks
├── storage/            # File storage management
├── crypto/             # Signing and verification
├── scheduler/          # Update scheduling
└── main.py             # Application entry point
```

**Update Process Flow**:
```python
# 1. Upload new firmware/app
POST /api/v1/ota/upload
{
    "version": "1.2.3",
    "type": "firmware|application",
    "file": "<binary-data>",
    "signature": "<digital-signature>",
    "changelog": "Bug fixes and improvements"
}

# 2. Create update campaign
POST /api/v1/ota/campaigns
{
    "name": "Firmware Update 1.2.3",
    "target_version": "1.2.3",
    "device_filter": {
        "current_version": "1.2.2",
        "device_type": "t-deck-pro"
    },
    "rollout_strategy": "staged",
    "rollout_percentage": 10
}

# 3. Device checks for updates
GET /api/v1/ota/check-updates?device_id=xxx&current_version=1.2.2

# 4. Device downloads update
GET /api/v1/ota/download/:updateId

# 5. Device reports update status
POST /api/v1/ota/status
{
    "device_id": "xxx",
    "update_id": "yyy",
    "status": "success|failed|in_progress",
    "error_message": "optional error details"
}
```

### 3. Meshtastic Bridge

**Purpose**: Bridges LoRa mesh networks with internet services

**Key Features**:
- Message routing and forwarding
- Node discovery and management
- Telemetry aggregation
- Emergency message prioritization
- Mesh network analytics

**Technology**: Python + asyncio + MQTT

**Directory Structure**:
```
src/meshtastic-bridge/
├── mesh/               # Mesh network handling
├── mqtt/               # MQTT client and handlers
├── routing/            # Message routing logic
├── telemetry/          # Telemetry collection
├── emergency/          # Emergency message handling
└── main.py             # Application entry point
```

**Message Flow**:
```python
# Mesh to Internet
mesh_message = {
    "from": "node_id_123",
    "to": "internet",
    "type": "text|telemetry|emergency",
    "payload": "message content",
    "timestamp": "2024-01-01T12:00:00Z",
    "hop_count": 3
}

# Internet to Mesh
internet_message = {
    "to": "node_id_456",
    "from": "server",
    "type": "command|notification",
    "payload": "command data",
    "priority": "high|normal|low"
}
```

### 4. Telemetry Collector

**Purpose**: Collects and analyzes device telemetry data

**Key Features**:
- Real-time data ingestion
- Time-series data storage
- Alerting and anomaly detection
- Performance analytics
- Health monitoring

**Technology**: Python + InfluxDB + Grafana

**Directory Structure**:
```
src/telemetry-collector/
├── ingestion/          # Data ingestion pipeline
├── storage/            # Time-series database
├── analytics/          # Data analysis
├── alerts/             # Alerting system
├── api/                # Query API
└── main.py             # Application entry point
```

**Telemetry Data Schema**:
```python
telemetry_point = {
    "device_id": "t-deck-pro-001",
    "timestamp": "2024-01-01T12:00:00Z",
    "metrics": {
        "battery_voltage": 3.7,
        "battery_percentage": 85,
        "cpu_usage": 45.2,
        "memory_usage": 67.8,
        "temperature": 23.5,
        "signal_strength": -65,
        "gps_latitude": 40.7128,
        "gps_longitude": -74.0060,
        "gps_accuracy": 5.0
    },
    "status": {
        "wifi_connected": True,
        "lora_active": True,
        "cellular_connected": False,
        "apps_running": ["meshtastic", "file_manager"]
    }
}
```

## API Development

### API Design Principles

1. **RESTful Design**: Follow REST conventions for resource-based APIs
2. **Versioning**: Use URL versioning (e.g., `/api/v1/`)
3. **Consistent Responses**: Standardized response format
4. **Error Handling**: Comprehensive error codes and messages
5. **Documentation**: OpenAPI 3.0 specifications
6. **Rate Limiting**: Protect against abuse
7. **Caching**: Optimize performance with appropriate caching

### Standard Response Format

```typescript
// Success Response
{
    "success": true,
    "data": {
        // Response data
    },
    "meta": {
        "timestamp": "2024-01-01T12:00:00Z",
        "request_id": "req_123456789",
        "version": "1.0.0"
    }
}

// Error Response
{
    "success": false,
    "error": {
        "code": "DEVICE_NOT_FOUND",
        "message": "Device with ID 'xxx' not found",
        "details": {
            "device_id": "xxx",
            "suggestion": "Check device ID and ensure device is registered"
        }
    },
    "meta": {
        "timestamp": "2024-01-01T12:00:00Z",
        "request_id": "req_123456789",
        "version": "1.0.0"
    }
}
```

### Authentication and Authorization

**JWT-Based Authentication**:
```typescript
// Login endpoint
POST /api/v1/auth/login
{
    "email": "user@example.com",
    "password": "secure_password"
}

// Response
{
    "success": true,
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIs...",
        "refresh_token": "eyJhbGciOiJIUzI1NiIs...",
        "expires_in": 3600,
        "user": {
            "id": "user_123",
            "email": "user@example.com",
            "role": "admin|user"
        }
    }
}

// Authorization header
Authorization: Bearer eyJhbGciOiJIUzI1NiIs...
```

**Device Authentication**:
```typescript
// Device registration
POST /api/v1/devices/register
{
    "device_id": "t-deck-pro-001",
    "device_type": "t-deck-pro",
    "hardware_version": "1.0",
    "firmware_version": "1.0.0",
    "public_key": "-----BEGIN PUBLIC KEY-----..."
}

// Device authentication
POST /api/v1/devices/auth
{
    "device_id": "t-deck-pro-001",
    "challenge_response": "signed_challenge_data"
}
```

### Rate Limiting

```typescript
// Rate limiting configuration
const rateLimits = {
    "auth": {
        "window": "15m",
        "max": 5,
        "message": "Too many authentication attempts"
    },
    "api": {
        "window": "1h",
        "max": 1000,
        "message": "API rate limit exceeded"
    },
    "ota": {
        "window": "1h",
        "max": 10,
        "message": "OTA download limit exceeded"
    }
};

// Rate limit headers
X-RateLimit-Limit: 1000
X-RateLimit-Remaining: 999
X-RateLimit-Reset: 1640995200
```

### WebSocket Integration

```typescript
// WebSocket connection
const ws = new WebSocket('wss://api.tdeckpro.local/ws');

// Authentication
ws.send(JSON.stringify({
    "type": "auth",
    "token": "jwt_token_here"
}));

// Subscribe to device events
ws.send(JSON.stringify({
    "type": "subscribe",
    "channels": ["device.t-deck-pro-001", "system.alerts"]
}));

// Real-time messages
{
    "type": "device.telemetry",
    "device_id": "t-deck-pro-001",
    "data": {
        "battery": 85,
        "temperature": 23.5
    },
    "timestamp": "2024-01-01T12:00:00Z"
}
```

## Database Design

### PostgreSQL Schema

**Users Table**:
```sql
CREATE TABLE users (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    email VARCHAR(255) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    role VARCHAR(50) DEFAULT 'user',
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    last_login TIMESTAMP,
    is_active BOOLEAN DEFAULT true
);
```

**Devices Table**:
```sql
CREATE TABLE devices (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id VARCHAR(100) UNIQUE NOT NULL,
    user_id UUID REFERENCES users(id),
    device_type VARCHAR(50) NOT NULL,
    hardware_version VARCHAR(20),
    firmware_version VARCHAR(20),
    public_key TEXT,
    last_seen TIMESTAMP,
    status VARCHAR(20) DEFAULT 'offline',
    config JSONB,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);
```

**Applications Table**:
```sql
CREATE TABLE applications (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL,
    version VARCHAR(20) NOT NULL,
    description TEXT,
    category VARCHAR(50),
    file_path VARCHAR(255),
    file_size BIGINT,
    checksum VARCHAR(64),
    metadata JSONB,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(name, version)
);
```

**Device Applications Table**:
```sql
CREATE TABLE device_applications (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id UUID REFERENCES devices(id),
    application_id UUID REFERENCES applications(id),
    installed_version VARCHAR(20),
    status VARCHAR(20) DEFAULT 'installed',
    config JSONB,
    installed_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(device_id, application_id)
);
```

**OTA Updates Table**:
```sql
CREATE TABLE ota_updates (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL,
    version VARCHAR(20) NOT NULL,
    type VARCHAR(20) NOT NULL, -- 'firmware' or 'application'
    file_path VARCHAR(255),
    file_size BIGINT,
    checksum VARCHAR(64),
    signature TEXT,
    changelog TEXT,
    is_active BOOLEAN DEFAULT true,
    created_at TIMESTAMP DEFAULT NOW()
);
```

**Update Campaigns Table**:
```sql
CREATE TABLE update_campaigns (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    name VARCHAR(100) NOT NULL,
    ota_update_id UUID REFERENCES ota_updates(id),
    device_filter JSONB,
    rollout_strategy VARCHAR(20) DEFAULT 'immediate',
    rollout_percentage INTEGER DEFAULT 100,
    start_time TIMESTAMP,
    end_time TIMESTAMP,
    status VARCHAR(20) DEFAULT 'draft',
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW()
);
```

**Device Update Status Table**:
```sql
CREATE TABLE device_update_status (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id UUID REFERENCES devices(id),
    campaign_id UUID REFERENCES update_campaigns(id),
    status VARCHAR(20) DEFAULT 'pending',
    started_at TIMESTAMP,
    completed_at TIMESTAMP,
    error_message TEXT,
    created_at TIMESTAMP DEFAULT NOW(),
    updated_at TIMESTAMP DEFAULT NOW(),
    UNIQUE(device_id, campaign_id)
);
```

**Telemetry Data Table**:
```sql
CREATE TABLE telemetry_data (
    id UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    device_id UUID REFERENCES devices(id),
    timestamp TIMESTAMP NOT NULL,
    metrics JSONB NOT NULL,
    status JSONB,
    created_at TIMESTAMP DEFAULT NOW()
);

-- Partition by time for performance
CREATE INDEX idx_telemetry_device_time ON telemetry_data (device_id, timestamp DESC);
CREATE INDEX idx_telemetry_timestamp ON telemetry_data (timestamp DESC);
```

### Redis Cache Structure

```redis
# Device sessions
device:session:{device_id} -> {
    "last_seen": "2024-01-01T12:00:00Z",
    "status": "online",
    "ip_address": "100.64.0.1"
}

# Rate limiting
rate_limit:api:{user_id}:{window} -> count
rate_limit:auth:{ip}:{window} -> count

# Real-time data
realtime:device:{device_id}:telemetry -> latest_telemetry_json
realtime:device:{device_id}:status -> current_status

# Caching
cache:user:{user_id} -> user_data_json
cache:device:{device_id} -> device_data_json
cache:apps:list -> applications_list_json
```

## Security Implementation

### VPN Security (Tailscale)

**Network Isolation**:
```yaml
# Tailscale ACL configuration
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

### API Security

**Input Validation**:
```typescript
import Joi from 'joi';

const deviceRegistrationSchema = Joi.object({
    device_id: Joi.string().pattern(/^[a-zA-Z0-9-_]+$/).required(),
    device_type: Joi.string().valid('t-deck-pro').required(),
    hardware_version: Joi.string().pattern(/^\d+\.\d+$/).required(),
    firmware_version: Joi.string().pattern(/^\d+\.\d+\.\d+$/).required(),
    public_key: Joi.string().required()
});

// Middleware
const validateInput = (schema) => (req, res, next) => {
    const { error } = schema.validate(req.body);
    if (error) {
        return res.status(400).json({
            success: false,
            error: {
                code: 'VALIDATION_ERROR',
                message: error.details[0].message
            }
        });
    }
    next();
};
```

**SQL Injection Prevention**:
```typescript
// Use parameterized queries
const getDevice = async (deviceId: string) => {
    const query = 'SELECT * FROM devices WHERE device_id = $1';
    const result = await db.query(query, [deviceId]);
    return result.rows[0];
};

// Input sanitization
import DOMPurify from 'isomorphic-dompurify';

const sanitizeInput = (input: string): string => {
    return DOMPurify.sanitize(input);
};
```

**Encryption**:
```typescript
import crypto from 'crypto';

class EncryptionService {
    private algorithm = 'aes-256-gcm';
    private key: Buffer;

    constructor(key: string) {
        this.key = crypto.scryptSync(key, 'salt', 32);
    }

    encrypt(text: string): string {
        const iv = crypto.randomBytes(16);
        const cipher = crypto.createCipher(this.algorithm, this.key);
        cipher.setAAD(Buffer.from('additional-data'));
        
        let encrypted = cipher.update(text, 'utf8', 'hex');
        encrypted += cipher.final('hex');
        
        const authTag = cipher.getAuthTag();
        
        return `${iv.toString('hex')}:${authTag.toString('hex')}:${encrypted}`;
    }

    decrypt(encryptedData: string): string {
        const [ivHex, authTagHex, encrypted] = encryptedData.split(':');
        const iv = Buffer.from(ivHex, 'hex');
        const authTag = Buffer.from(authTagHex, 'hex');
        
        const decipher = crypto.createDecipher(this.algorithm, this.key);
        decipher.setAAD(Buffer.from('additional-data'));
        decipher.setAuthTag(authTag);
        
        let decrypted = decipher.update(encrypted, 'hex', 'utf8');
        decrypted += decipher.final('utf8');
        
        return decrypted;
    }
}
```

### Device Authentication

**Public Key Infrastructure**:
```typescript
import crypto from 'crypto';

class DeviceAuth {
    static generateKeyPair() {
        return crypto.generateKeyPairSync('rsa', {
            modulusLength: 2048,
            publicKeyEncoding: {
                type: 'spki',
                format: 'pem'
            },
            privateKeyEncoding: {
                type: 'pkcs8',
                format: 'pem'
            }
        });
    }

    static createChallenge(): string {
        return crypto.randomBytes(32).toString('hex');
    }

    static verifyChallenge(
        challenge: string,
        signature: string,
        publicKey: string
    ): boolean {
        const verify = crypto.createVerify('SHA256');
        verify.update(challenge);
        verify.end();
        
        return verify.verify(publicKey, signature, 'hex');
    }
}
```

## Integration with T-Deck-Pro OS

### Device Communication Protocol

**HTTP Client Implementation** (for T-Deck-Pro OS):
```cpp
// src/core/communication/server_client.h
#pragma once

#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "core/utils/logger.h"

class ServerClient {
private:
    HTTPClient http;
    WiFiClientSecure client;
    String baseUrl;
    String deviceId;
    String authToken;
    
public:
    ServerClient(const String& url, const String& deviceId);
    
    // Authentication
    bool authenticate();
    bool refreshToken();
    
    // Device management
    bool registerDevice(const JsonObject& deviceInfo);
    bool updateDeviceStatus(const JsonObject& status);
    bool getDeviceConfig(JsonObject& config);
    
    // Application management
    bool getAvailableApps(JsonArray& apps);
    bool downloadApp(const String& appId, const String& version);
    bool reportAppStatus(const String& appId, const String& status);
    
    // OTA updates
    bool checkForUpdates(JsonObject& updateInfo);
    bool downloadUpdate(const String& updateId);
    bool reportUpdateStatus(const String& updateId, const String& status);
    
    // Telemetry
    bool sendTelemetry(const JsonObject& telemetry);
    
    // Utility
    bool isConnected();
    void setAuthToken(const String& token);
};
```

**Implementation**:
```cpp
// src/core/communication/server_client.cpp
#include "server_client.h"

ServerClient::ServerClient(const String& url, const String& deviceId) 
    : baseUrl(url), deviceId(deviceId) {
    client.setInsecure(); // For development - use proper certificates in production
}

bool ServerClient::authenticate() {
    http.begin(client, baseUrl + "/api/v1/devices/auth");
    http.addHeader("Content-Type", "application/json");
    
    // Create challenge response using device private key
    JsonDocument doc;
    doc["device_id"] = deviceId;
    doc["challenge_response"] = createChallengeResponse();
    
    String payload;
    serializeJson(doc, payload);
    
    int httpCode = http.POST(payload);
    
    if (httpCode == 200) {
        String response = http.getString();
        JsonDocument responseDoc;
        deserializeJson(responseDoc, response);
        
        if (responseDoc["success"]) {
            authToken = responseDoc["data"]["access_token"].as<String>();
            Logger::info("Device authenticated successfully");
            return true;
        }
    }
    
    Logger::error("Device authentication failed: " + String(httpCode));
    return false;
}

bool ServerClient::sendTelemetry(const JsonObject& telemetry) {
    if (!isConnected()) {
        Logger::warning("Not connected to server, caching telemetry");
        // Cache telemetry for later transmission
        return false;
    }
    
    http.begin(client, baseUrl + "/api/v1/telemetry");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + authToken);
    
    String payload;
    serializeJson(telemetry, payload);
    
    int httpCode = http.POST(payload);
    
    if (httpCode == 200 || httpCode == 201) {
        Logger::debug("Telemetry sent successfully");
        return true;
    }
    
    Logger::error("Failed to send telemetry: " + String(httpCode));
    return false;
}

bool ServerClient::checkForUpdates(JsonObject& updateInfo) {
    http.begin(client, baseUrl + "/api/v1/ota/check-updates?device_id=" + deviceId);
    http.addHeader("Authorization", "Bearer " + authToken);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
        String response = http.getString();
        JsonDocument responseDoc;
        deserializeJson(responseDoc, response);
        
        if (responseDoc["success"] && responseDoc["data"]["has_update"]) {
            updateInfo.set(responseDoc["data"]);