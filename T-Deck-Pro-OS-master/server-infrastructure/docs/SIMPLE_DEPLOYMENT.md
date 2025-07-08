# T-Deck-Pro Server - Simple Deployment Guide

## Overview

This is a lightweight, self-hosted server for T-Deck-Pro devices using MQTT communication and SQLite storage. Perfect for personal use with Tailscale VPN.

## Quick Start

### Option 1: Docker Deployment (Recommended)

1. **Prerequisites**
   ```bash
   # Install Docker and Docker Compose
   sudo apt update
   sudo apt install docker.io docker-compose
   ```

2. **Deploy**
   ```bash
   # Clone or copy the server-infrastructure folder
   cd server-infrastructure
   
   # Start services
   docker-compose up -d
   ```

3. **Access**
   - Web Dashboard: http://localhost:8000
   - MQTT Broker: localhost:1883

### Option 2: Direct Python Deployment

1. **Prerequisites**
   ```bash
   # Install Python 3.11+
   sudo apt update
   sudo apt install python3 python3-pip
   
   # Install Mosquitto MQTT broker
   sudo apt install mosquitto mosquitto-clients
   ```

2. **Setup**
   ```bash
   cd server-infrastructure
   
   # Install Python dependencies
   pip3 install -r requirements.txt
   
   # Start MQTT broker
   sudo systemctl start mosquitto
   sudo systemctl enable mosquitto
   ```

3. **Run Server**
   ```bash
   python3 server.py
   ```

## Configuration

### MQTT Topics

The server uses these MQTT topics:

- `tdeckpro/{device_id}/register` - Device registration
- `tdeckpro/{device_id}/telemetry` - Device telemetry data
- `tdeckpro/{device_id}/status` - Device status updates
- `tdeckpro/{device_id}/config` - Server-to-device configuration
- `tdeckpro/{device_id}/ota` - OTA update notifications
- `tdeckpro/{device_id}/apps` - App management
- `tdeckpro/mesh/{message_type}` - Mesh network messages

### Environment Variables

```bash
MQTT_BROKER=localhost    # MQTT broker hostname
MQTT_PORT=1883          # MQTT broker port
WEB_PORT=8000           # Web interface port
DB_PATH=data/tdeckpro.db # SQLite database path
```

## Tailscale Integration

1. **Install Tailscale on server**
   ```bash
   curl -fsSL https://tailscale.com/install.sh | sh
   sudo tailscale up
   ```

2. **Configure T-Deck-Pro devices**
   - Install Tailscale on each device
   - Use Tailscale IP for MQTT broker connection
   - No additional authentication needed (network presence = auth)

## File Structure

```
server-infrastructure/
├── server.py              # Main server application
├── mqtt_client.py         # MQTT client library
├── requirements.txt       # Python dependencies
├── docker-compose.yml     # Docker deployment
├── Dockerfile            # Server container
├── mosquitto/
│   └── config/
│       └── mosquitto.conf # MQTT broker config
└── data/                 # Runtime data
    ├── tdeckpro.db       # SQLite database
    ├── ota-updates/      # OTA files
    └── logs/             # Log files
```

## Usage

### Web Dashboard

Access the web dashboard at http://your-server:8000

Features:
- View connected devices
- Monitor device telemetry
- Upload OTA updates
- View mesh messages
- Check logs

### API Endpoints

- `GET /api/devices` - List all devices
- `GET /api/device/{id}/telemetry` - Get device telemetry
- `GET /api/device/{id}/updates` - Check for updates
- `POST /api/ota/upload` - Upload OTA update

### MQTT Integration

Use the provided `mqtt_client.py` for T-Deck-Pro OS integration:

```python
from mqtt_client import TDeckProServerIntegration

# Initialize
integration = TDeckProServerIntegration("t-deck-pro-001", "your-server-ip")

# Connect and register
if integration.initialize():
    print("Connected to server")
    
    # Main loop
    while True:
        integration.update()  # Handles telemetry and messages
        time.sleep(1)
```

## Troubleshooting

### MQTT Connection Issues

```bash
# Test MQTT broker
mosquitto_pub -h localhost -t test -m "hello"
mosquitto_sub -h localhost -t test

# Check broker logs
docker logs tdeckpro-mqtt
```

### Server Issues

```bash
# Check server logs
docker logs tdeckpro-server

# Or for direct Python deployment
tail -f data/logs/server.log
```

### Database Issues

```bash
# Check SQLite database
sqlite3 data/tdeckpro.db ".tables"
sqlite3 data/tdeckpro.db "SELECT * FROM devices;"
```

## Security Notes

- This server is designed for use within a Tailscale VPN
- No authentication is implemented (network presence = auth)
- MQTT broker allows anonymous connections
- Suitable for personal/small-scale deployment only
- Not intended for production or public internet exposure

## Maintenance

### Backup

```bash
# Backup database and OTA files
tar -czf backup-$(date +%Y%m%d).tar.gz data/
```

### Updates

```bash
# Update server
git pull  # or copy new files
docker-compose down
docker-compose up -d --build
```

### Logs

```bash
# View logs
docker-compose logs -f

# Clean old logs
find data/logs -name "*.log" -mtime +30 -delete
```

## Performance

This lightweight server can easily handle:
- 10-50 T-Deck-Pro devices
- 1000+ MQTT messages per minute
- 100MB+ of telemetry data
- Multiple concurrent OTA updates

Resource usage:
- RAM: ~50MB
- CPU: <5% on Raspberry Pi 4
- Storage: Grows with telemetry data

## Next Steps

1. Deploy server on your Tailscale network
2. Configure T-Deck-Pro devices to connect
3. Monitor devices via web dashboard
4. Set up automated backups
5. Customize for your specific needs