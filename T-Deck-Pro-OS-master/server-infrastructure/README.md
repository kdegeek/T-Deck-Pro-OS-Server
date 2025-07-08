# T-Deck-Pro Lightweight Server Infrastructure

## Overview

A simple, lightweight server infrastructure for T-Deck-Pro devices focused on ease of use, speed, and functionality. Designed for self-hosted deployment with minimal complexity.

## Architecture

```
T-Deck-Pro Devices ←→ Tailscale VPN ←→ MQTT Broker ←→ Simple Server
                                                        ├── SQLite Database
                                                        ├── File Storage
                                                        └── Web Interface
```

## Core Components

### 1. MQTT Broker (Mosquitto)
- Primary communication method between devices and server
- Lightweight, reliable message passing
- Built-in authentication and topic-based routing

### 2. Simple Server (Python/FastAPI)
- Handles MQTT message processing
- SQLite database for all data storage
- File-based storage for OTA updates and apps
- Optional web interface for management

### 3. SQLite Database
- Single file database - perfect for lightweight deployment
- No complex setup or maintenance
- Handles all device data, telemetry, and configuration

### 4. Tailscale VPN
- Provides secure network access
- Device authentication through network presence
- No additional auth layers needed

## Quick Start

```bash
# 1. Clone and setup
git clone <repository-url>
cd server-infrastructure

# 2. Install dependencies
pip install -r requirements.txt

# 3. Configure Tailscale
sudo tailscale up --auth-key=<your-auth-key>

# 4. Start MQTT broker
docker run -d --name mosquitto -p 1883:1883 eclipse-mosquitto

# 5. Start server
python server.py

# 6. Access web interface
open http://localhost:8000
```

## File Structure

```
server-infrastructure/
├── server.py                  # Main server application
├── mqtt_handler.py           # MQTT message processing
├── database.py               # SQLite database operations
├── web_interface.py          # Optional web UI
├── config.py                 # Configuration
├── requirements.txt          # Python dependencies
├── data/
│   ├── tdeckpro.db           # SQLite database
│   ├── ota-updates/          # Firmware and app files
│   └── logs/                 # Simple log files
└── static/                   # Web interface assets
```

## Features

- **MQTT Communication**: All device communication via MQTT topics
- **SQLite Storage**: Single-file database for all data
- **File-based OTA**: Simple file storage for updates
- **Tailscale Security**: Network-level authentication
- **Web Interface**: Simple management dashboard
- **Zero Configuration**: Works out of the box
- **Lightweight**: Minimal resource usage
- **Self-contained**: No external dependencies

## MQTT Topics

```
tdeckpro/{device_id}/telemetry    # Device sends telemetry data
tdeckpro/{device_id}/status       # Device status updates
tdeckpro/{device_id}/config       # Server sends configuration
tdeckpro/{device_id}/ota          # OTA update notifications
tdeckpro/{device_id}/apps         # App management
tdeckpro/mesh/messages            # Meshtastic message bridge
```

## Device Integration

T-Deck-Pro devices connect via MQTT and authenticate through Tailscale network presence. No complex authentication flows needed.

This infrastructure prioritizes simplicity and functionality over enterprise features.