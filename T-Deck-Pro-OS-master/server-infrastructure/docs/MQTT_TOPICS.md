# T-Deck-Pro MQTT Topics Reference

## Overview

The T-Deck-Pro server uses MQTT for all device communication. This document describes the topic structure and message formats.

## Topic Structure

All topics follow the pattern: `tdeckpro/{device_id}/{message_type}`

### Device Registration

**Topic:** `tdeckpro/{device_id}/register`
**Direction:** Device → Server
**QoS:** 1 (At least once)

```json
{
  "device_type": "t-deck-pro",
  "firmware_version": "1.0.0",
  "hardware_version": "1.0",
  "capabilities": {
    "wifi": true,
    "lora": true,
    "cellular": true,
    "bluetooth": true,
    "gps": true,
    "eink_display": true
  },
  "config": {
    "timezone": "UTC",
    "language": "en"
  }
}
```

### Device Telemetry

**Topic:** `tdeckpro/{device_id}/telemetry`
**Direction:** Device → Server
**QoS:** 0 (Fire and forget)
**Frequency:** Every 5 minutes (configurable)

```json
{
  "timestamp": "2025-01-08T21:30:00Z",
  "battery_percentage": 85,
  "temperature": 23.5,
  "cpu_usage": 45.2,
  "memory_usage": 67.8,
  "signal_strength": -65,
  "gps_latitude": 40.7128,
  "gps_longitude": -74.0060,
  "wifi_connected": true,
  "lora_active": true,
  "cellular_connected": false,
  "running_apps": ["meshtastic", "file_manager"]
}
```

### Device Status

**Topic:** `tdeckpro/{device_id}/status`
**Direction:** Device → Server
**QoS:** 1 (At least once)

```json
{
  "status": "online|offline|sleeping|updating",
  "timestamp": "2025-01-08T21:30:00Z",
  "additional_info": {
    "reason": "user_initiated",
    "expected_duration": 300
  }
}
```

### Configuration Updates

**Topic:** `tdeckpro/{device_id}/config`
**Direction:** Server → Device
**QoS:** 1 (At least once)

```json
{
  "server_time": "2025-01-08T21:30:00Z",
  "update_interval": 300,
  "auto_update": true,
  "timezone": "America/New_York",
  "language": "en",
  "display_settings": {
    "brightness": 80,
    "sleep_timeout": 60
  },
  "network_settings": {
    "wifi_auto_connect": true,
    "lora_frequency": 915.0,
    "cellular_apn": "internet"
  }
}
```

### OTA Updates

**Topic:** `tdeckpro/{device_id}/ota`
**Direction:** Server → Device
**QoS:** 1 (At least once)

```json
{
  "available": true,
  "type": "firmware|app",
  "version": "1.1.0",
  "filename": "firmware-v1.1.0.bin",
  "checksum": "sha256:abc123...",
  "download_url": "/ota/download/firmware-v1.1.0.bin",
  "size_bytes": 1048576,
  "release_notes": "Bug fixes and performance improvements"
}
```

### App Management

**Topic:** `tdeckpro/{device_id}/apps`
**Direction:** Server → Device
**QoS:** 1 (At least once)

```json
{
  "action": "install|remove|update|list",
  "app_id": "weather_app",
  "app_name": "Weather App",
  "version": "2.0.0",
  "filename": "weather_app_v2.0.0.bin",
  "checksum": "sha256:def456...",
  "download_url": "/apps/download/weather_app_v2.0.0.bin",
  "dependencies": ["network_lib", "display_lib"]
}
```

### Mesh Network Messages

**Topic:** `tdeckpro/mesh/{message_type}`
**Direction:** Device → Server
**QoS:** 0 (Fire and forget)

```json
{
  "from_node": "t-deck-pro-001",
  "to_node": "t-deck-pro-002|broadcast",
  "message_type": "text|position|telemetry|emergency",
  "payload": {
    "text": "Hello from node 1",
    "latitude": 40.7128,
    "longitude": -74.0060,
    "battery": 85,
    "emergency_type": "medical"
  },
  "timestamp": "2025-01-08T21:30:00Z",
  "hop_count": 1,
  "rssi": -65
}
```

## Message Types

### Standard Message Types

- `register` - Device registration and capabilities
- `telemetry` - Periodic device telemetry data
- `status` - Device status changes
- `config` - Configuration updates from server
- `ota` - Over-the-air update notifications
- `apps` - Application management

### Mesh Message Types

- `text` - Text messages between nodes
- `position` - GPS position updates
- `telemetry` - Mesh network telemetry
- `emergency` - Emergency alerts
- `routing` - Mesh routing information

## QoS Levels

- **QoS 0 (Fire and forget):** Used for telemetry and mesh messages where occasional loss is acceptable
- **QoS 1 (At least once):** Used for important messages like registration, status, config, and OTA
- **QoS 2 (Exactly once):** Not used to keep things simple

## Retained Messages

The server may retain certain messages:

- Device status (last known status)
- Configuration (current device config)
- OTA availability (pending updates)

## Wildcards

Useful wildcard subscriptions:

- `tdeckpro/+/telemetry` - All device telemetry
- `tdeckpro/+/status` - All device status updates
- `tdeckpro/mesh/+` - All mesh messages
- `tdeckpro/{device_id}/+` - All messages for specific device

## Error Handling

### Invalid Messages

If a message cannot be parsed or is invalid:

```json
{
  "error": "invalid_format",
  "message": "JSON parsing failed",
  "original_topic": "tdeckpro/device-001/telemetry",
  "timestamp": "2025-01-08T21:30:00Z"
}
```

### Device Offline

When a device goes offline, the server publishes:

**Topic:** `tdeckpro/{device_id}/status`
```json
{
  "status": "offline",
  "timestamp": "2025-01-08T21:30:00Z",
  "reason": "timeout",
  "last_seen": "2025-01-08T21:25:00Z"
}
```

## Security Considerations

- All communication happens within Tailscale VPN
- No authentication at MQTT level (network presence = auth)
- Device ID should be unique and non-guessable
- Sensitive data should be avoided in telemetry

## Implementation Examples

### Python Client

```python
import paho.mqtt.client as mqtt
import json

def on_connect(client, userdata, flags, rc):
    client.subscribe("tdeckpro/+/telemetry")

def on_message(client, userdata, msg):
    data = json.loads(msg.payload.decode())
    print(f"Telemetry from {msg.topic}: {data}")

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.connect("localhost", 1883, 60)
client.loop_forever()
```

### Arduino/ESP32 Client

```cpp
#include <PubSubClient.h>
#include <ArduinoJson.h>

void callback(char* topic, byte* payload, unsigned int length) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload, length);
    
    if (String(topic).endsWith("/config")) {
        handleConfig(doc);
    }
}

void sendTelemetry() {
    DynamicJsonDocument doc(1024);
    doc["battery_percentage"] = getBatteryLevel();
    doc["temperature"] = getTemperature();
    
    String payload;
    serializeJson(doc, payload);
    
    client.publish("tdeckpro/my-device/telemetry", payload.c_str());
}
```

## Testing

### MQTT Client Tools

```bash
# Subscribe to all device messages
mosquitto_sub -h localhost -t "tdeckpro/+/+"

# Send test telemetry
mosquitto_pub -h localhost -t "tdeckpro/test-device/telemetry" \
  -m '{"battery_percentage": 75, "temperature": 22.1}'

# Send device registration
mosquitto_pub -h localhost -t "tdeckpro/test-device/register" \
  -m '{"device_type": "t-deck-pro", "firmware_version": "1.0.0"}'
```

### Web Interface

The server provides a web interface at http://localhost:8000 for:
- Viewing live MQTT messages
- Testing message publishing
- Monitoring device status
- Managing OTA updates