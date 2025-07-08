# T-Deck-Pro Server Infrastructure API Reference

## Table of Contents

1. [Authentication API](#authentication-api)
2. [Device Management API](#device-management-api)
3. [Application Management API](#application-management-api)
4. [OTA Update API](#ota-update-api)
5. [Telemetry API](#telemetry-api)
6. [Meshtastic Bridge API](#meshtastic-bridge-api)
7. [User Management API](#user-management-api)
8. [System API](#system-api)
9. [WebSocket API](#websocket-api)
10. [Error Codes](#error-codes)

## Base URL

All API endpoints are relative to the base URL:
```
https://api.tdeckpro.local/api/v1
```

## Authentication

All API requests (except authentication endpoints) require a valid JWT token in the Authorization header:
```
Authorization: Bearer <jwt_token>
```

## Standard Response Format

### Success Response
```json
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
```

### Error Response
```json
{
    "success": false,
    "error": {
        "code": "ERROR_CODE",
        "message": "Human readable error message",
        "details": {
            // Additional error details
        }
    },
    "meta": {
        "timestamp": "2024-01-01T12:00:00Z",
        "request_id": "req_123456789",
        "version": "1.0.0"
    }
}
```

## Authentication API

### Login

**Endpoint**: `POST /auth/login`

**Description**: Authenticate user and receive access token

**Request Body**:
```json
{
    "email": "user@example.com",
    "password": "secure_password"
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIs...",
        "refresh_token": "eyJhbGciOiJIUzI1NiIs...",
        "expires_in": 3600,
        "user": {
            "id": "user_123",
            "email": "user@example.com",
            "role": "admin|user",
            "created_at": "2024-01-01T12:00:00Z"
        }
    }
}
```

**Error Codes**:
- `INVALID_CREDENTIALS`: Invalid email or password
- `ACCOUNT_LOCKED`: Account temporarily locked due to failed attempts
- `ACCOUNT_DISABLED`: Account has been disabled

### Refresh Token

**Endpoint**: `POST /auth/refresh`

**Description**: Refresh access token using refresh token

**Request Body**:
```json
{
    "refresh_token": "eyJhbGciOiJIUzI1NiIs..."
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIs...",
        "expires_in": 3600
    }
}
```

### Device Authentication

**Endpoint**: `POST /devices/auth`

**Description**: Authenticate device using public key cryptography

**Request Body**:
```json
{
    "device_id": "t-deck-pro-001",
    "challenge_response": "signed_challenge_data_in_hex"
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "access_token": "eyJhbGciOiJIUzI1NiIs...",
        "expires_in": 86400,
        "device": {
            "id": "device_uuid",
            "device_id": "t-deck-pro-001",
            "status": "online",
            "last_seen": "2024-01-01T12:00:00Z"
        }
    }
}
```

## Device Management API

### Register Device

**Endpoint**: `POST /devices/register`

**Description**: Register a new T-Deck-Pro device

**Request Body**:
```json
{
    "device_id": "t-deck-pro-001",
    "device_type": "t-deck-pro",
    "hardware_version": "1.0",
    "firmware_version": "1.0.0",
    "public_key": "-----BEGIN PUBLIC KEY-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA...\n-----END PUBLIC KEY-----",
    "capabilities": {
        "wifi": true,
        "lora": true,
        "cellular": true,
        "bluetooth": true,
        "gps": true,
        "eink_display": true
    },
    "initial_config": {
        "timezone": "UTC",
        "language": "en",
        "auto_update": true
    }
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "device": {
            "id": "device_uuid",
            "device_id": "t-deck-pro-001",
            "status": "registered",
            "registration_code": "REG123456",
            "created_at": "2024-01-01T12:00:00Z"
        },
        "initial_apps": [
            {
                "app_id": "meshtastic",
                "version": "1.0.0",
                "auto_install": true
            }
        ]
    }
}
```

### Get Device

**Endpoint**: `GET /devices/{device_id}`

**Description**: Get device information and current status

**Response**:
```json
{
    "success": true,
    "data": {
        "device": {
            "id": "device_uuid",
            "device_id": "t-deck-pro-001",
            "device_type": "t-deck-pro",
            "hardware_version": "1.0",
            "firmware_version": "1.0.0",
            "status": "online|offline|maintenance",
            "last_seen": "2024-01-01T12:00:00Z",
            "location": {
                "latitude": 40.7128,
                "longitude": -74.0060,
                "accuracy": 5.0,
                "timestamp": "2024-01-01T12:00:00Z"
            },
            "telemetry": {
                "battery_percentage": 85,
                "temperature": 23.5,
                "signal_strength": -65,
                "memory_usage": 67.8,
                "cpu_usage": 45.2
            },
            "config": {
                "timezone": "UTC",
                "language": "en",
                "auto_update": true,
                "mesh_enabled": true
            },
            "installed_apps": [
                {
                    "app_id": "meshtastic",
                    "version": "1.0.0",
                    "status": "running"
                }
            ]
        }
    }
}
```

### Update Device Configuration

**Endpoint**: `PUT /devices/{device_id}/config`

**Description**: Update device configuration

**Request Body**:
```json
{
    "config": {
        "timezone": "America/New_York",
        "language": "en",
        "auto_update": false,
        "mesh_enabled": true,
        "display_brightness": 80,
        "sleep_timeout": 300,
        "wifi_auto_connect": true,
        "lora_frequency": 915.0,
        "cellular_apn": "internet"
    }
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "config_updated": true,
        "restart_required": false,
        "applied_at": "2024-01-01T12:00:00Z"
    }
}
```

### Update Device Status

**Endpoint**: `POST /devices/{device_id}/status`

**Description**: Update device status and telemetry

**Request Body**:
```json
{
    "status": "online|offline|maintenance|error",
    "telemetry": {
        "battery_voltage": 3.7,
        "battery_percentage": 85,
        "temperature": 23.5,
        "cpu_usage": 45.2,
        "memory_usage": 67.8,
        "signal_strength": -65,
        "gps_latitude": 40.7128,
        "gps_longitude": -74.0060,
        "gps_accuracy": 5.0
    },
    "running_apps": ["meshtastic", "file_manager"],
    "error_logs": [
        {
            "level": "error",
            "message": "Failed to connect to WiFi",
            "timestamp": "2024-01-01T12:00:00Z",
            "component": "wifi_manager"
        }
    ]
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "status_updated": true,
        "alerts_generated": 0,
        "next_checkin": "2024-01-01T12:05:00Z"
    }
}
```

### List Devices

**Endpoint**: `GET /devices`

**Description**: List all devices for the authenticated user

**Query Parameters**:
- `status`: Filter by status (online, offline, maintenance, error)
- `device_type`: Filter by device type
- `limit`: Number of results (default: 50, max: 100)
- `offset`: Pagination offset (default: 0)
- `sort`: Sort field (created_at, last_seen, device_id)
- `order`: Sort order (asc, desc)

**Response**:
```json
{
    "success": true,
    "data": {
        "devices": [
            {
                "id": "device_uuid",
                "device_id": "t-deck-pro-001",
                "status": "online",
                "last_seen": "2024-01-01T12:00:00Z",
                "firmware_version": "1.0.0",
                "battery_percentage": 85
            }
        ],
        "pagination": {
            "total": 1,
            "limit": 50,
            "offset": 0,
            "has_more": false
        }
    }
}
```

### Delete Device

**Endpoint**: `DELETE /devices/{device_id}`

**Description**: Unregister and delete device

**Response**:
```json
{
    "success": true,
    "data": {
        "device_deleted": true,
        "cleanup_completed": true
    }
}
```

## Application Management API

### List Available Applications

**Endpoint**: `GET /apps`

**Description**: Get list of available applications

**Query Parameters**:
- `category`: Filter by category (communication, utility, entertainment, etc.)
- `compatible_version`: Filter by firmware compatibility
- `featured`: Show only featured apps (true/false)

**Response**:
```json
{
    "success": true,
    "data": {
        "applications": [
            {
                "id": "meshtastic",
                "name": "Meshtastic",
                "version": "1.0.0",
                "description": "Mesh networking communication app",
                "category": "communication",
                "size": 2048576,
                "icon_url": "https://api.tdeckpro.local/static/icons/meshtastic.png",
                "screenshots": [
                    "https://api.tdeckpro.local/static/screenshots/meshtastic_1.png"
                ],
                "compatibility": {
                    "min_firmware": "1.0.0",
                    "max_firmware": "2.0.0",
                    "hardware_requirements": {
                        "lora": true,
                        "gps": false
                    }
                },
                "permissions": [
                    "network_access",
                    "location_access",
                    "storage_write"
                ],
                "featured": true,
                "rating": 4.8,
                "downloads": 1250
            }
        ]
    }
}
```

### Get Application Details

**Endpoint**: `GET /apps/{app_id}`

**Description**: Get detailed information about a specific application

**Response**:
```json
{
    "success": true,
    "data": {
        "application": {
            "id": "meshtastic",
            "name": "Meshtastic",
            "version": "1.0.0",
            "description": "Comprehensive mesh networking communication application",
            "long_description": "Detailed description...",
            "category": "communication",
            "developer": "T-Deck-Pro Team",
            "size": 2048576,
            "checksum": "sha256:abc123...",
            "changelog": "Initial release with basic mesh functionality",
            "compatibility": {
                "min_firmware": "1.0.0",
                "max_firmware": "2.0.0",
                "hardware_requirements": {
                    "lora": true,
                    "gps": false,
                    "min_memory": 512000
                }
            },
            "permissions": [
                "network_access",
                "location_access",
                "storage_write"
            ],
            "configuration_schema": {
                "type": "object",
                "properties": {
                    "node_name": {
                        "type": "string",
                        "default": "T-Deck-Pro"
                    },
                    "frequency": {
                        "type": "number",
                        "default": 915.0,
                        "min": 902.0,
                        "max": 928.0
                    }
                }
            },
            "created_at": "2024-01-01T12:00:00Z",
            "updated_at": "2024-01-01T12:00:00Z"
        }
    }
}
```

### Install Application

**Endpoint**: `POST /devices/{device_id}/apps/{app_id}/install`

**Description**: Install application on device

**Request Body**:
```json
{
    "version": "1.0.0",
    "config": {
        "node_name": "My T-Deck Pro",
        "frequency": 915.0
    },
    "auto_start": true
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "installation_id": "install_uuid",
        "status": "queued",
        "estimated_time": 120,
        "download_url": "https://api.tdeckpro.local/downloads/apps/meshtastic/1.0.0",
        "checksum": "sha256:abc123..."
    }
}
```

### Get Installation Status

**Endpoint**: `GET /devices/{device_id}/apps/{app_id}/install/{installation_id}`

**Description**: Check installation progress

**Response**:
```json
{
    "success": true,
    "data": {
        "installation": {
            "id": "install_uuid",
            "status": "downloading|installing|completed|failed",
            "progress": 75,
            "error_message": null,
            "started_at": "2024-01-01T12:00:00Z",
            "completed_at": null,
            "estimated_remaining": 30
        }
    }
}
```

### Uninstall Application

**Endpoint**: `DELETE /devices/{device_id}/apps/{app_id}`

**Description**: Uninstall application from device

**Response**:
```json
{
    "success": true,
    "data": {
        "uninstallation_id": "uninstall_uuid",
        "status": "queued",
        "preserve_data": false
    }
}
```

### Update Application Configuration

**Endpoint**: `PUT /devices/{device_id}/apps/{app_id}/config`

**Description**: Update application configuration

**Request Body**:
```json
{
    "config": {
        "node_name": "Updated Name",
        "frequency": 920.0
    },
    "restart_app": true
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "config_updated": true,
        "restart_required": true,
        "applied_at": "2024-01-01T12:00:00Z"
    }
}
```

## OTA Update API

### Check for Updates

**Endpoint**: `GET /ota/check-updates`

**Description**: Check if updates are available for device

**Query Parameters**:
- `device_id`: Device identifier
- `current_firmware`: Current firmware version
- `current_apps`: JSON array of installed apps with versions

**Response**:
```json
{
    "success": true,
    "data": {
        "has_updates": true,
        "firmware_update": {
            "available": true,
            "version": "1.1.0",
            "size": 8388608,
            "critical": false,
            "changelog": "Bug fixes and performance improvements",
            "download_url": "https://api.tdeckpro.local/ota/firmware/1.1.0",
            "checksum": "sha256:def456..."
        },
        "app_updates": [
            {
                "app_id": "meshtastic",
                "current_version": "1.0.0",
                "available_version": "1.1.0",
                "size": 2097152,
                "changelog": "Added new mesh features",
                "download_url": "https://api.tdeckpro.local/ota/apps/meshtastic/1.1.0",
                "checksum": "sha256:ghi789..."
            }
        ],
        "total_download_size": 10485760,
        "estimated_install_time": 300
    }
}
```

### Download Update

**Endpoint**: `GET /ota/download/{update_type}/{update_id}`

**Description**: Download firmware or application update

**Parameters**:
- `update_type`: "firmware" or "app"
- `update_id`: Update identifier

**Headers**:
- `Range`: For resumable downloads (optional)

**Response**: Binary data stream with appropriate headers

### Report Update Status

**Endpoint**: `POST /ota/status`

**Description**: Report update installation status

**Request Body**:
```json
{
    "device_id": "t-deck-pro-001",
    "update_type": "firmware|app",
    "update_id": "1.1.0",
    "app_id": "meshtastic",
    "status": "downloading|installing|completed|failed|rolled_back",
    "progress": 100,
    "error_message": null,
    "installation_time": 180,
    "previous_version": "1.0.0"
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "status_recorded": true,
        "next_check_in": "2024-01-01T18:00:00Z"
    }
}
```

### List Update History

**Endpoint**: `GET /devices/{device_id}/ota/history`

**Description**: Get update history for device

**Response**:
```json
{
    "success": true,
    "data": {
        "updates": [
            {
                "id": "update_uuid",
                "type": "firmware",
                "from_version": "1.0.0",
                "to_version": "1.1.0",
                "status": "completed",
                "started_at": "2024-01-01T12:00:00Z",
                "completed_at": "2024-01-01T12:03:00Z",
                "installation_time": 180
            }
        ]
    }
}
```

## Telemetry API

### Send Telemetry Data

**Endpoint**: `POST /telemetry`

**Description**: Send device telemetry data

**Request Body**:
```json
{
    "device_id": "t-deck-pro-001",
    "timestamp": "2024-01-01T12:00:00Z",
    "metrics": {
        "battery_voltage": 3.7,
        "battery_percentage": 85,
        "battery_charging": false,
        "temperature": 23.5,
        "humidity": 45.0,
        "cpu_usage": 45.2,
        "memory_usage": 67.8,
        "storage_usage": 34.5,
        "wifi_signal": -45,
        "cellular_signal": -65,
        "lora_signal": -80,
        "gps_latitude": 40.7128,
        "gps_longitude": -74.0060,
        "gps_accuracy": 5.0,
        "gps_satellites": 8
    },
    "status": {
        "wifi_connected": true,
        "cellular_connected": false,
        "lora_active": true,
        "bluetooth_active": false,
        "gps_active": true,
        "apps_running": ["meshtastic", "file_manager"],
        "system_errors": 0,
        "uptime": 86400
    },
    "events": [
        {
            "type": "app_started",
            "app_id": "meshtastic",
            "timestamp": "2024-01-01T11:55:00Z"
        }
    ]
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "telemetry_recorded": true,
        "alerts_generated": 0,
        "next_expected": "2024-01-01T12:05:00Z"
    }
}
```

### Get Telemetry Data

**Endpoint**: `GET /devices/{device_id}/telemetry`

**Description**: Retrieve historical telemetry data

**Query Parameters**:
- `start_time`: Start timestamp (ISO 8601)
- `end_time`: End timestamp (ISO 8601)
- `metrics`: Comma-separated list of metrics to include
- `interval`: Data aggregation interval (1m, 5m, 1h, 1d)
- `limit`: Maximum number of data points

**Response**:
```json
{
    "success": true,
    "data": {
        "telemetry": [
            {
                "timestamp": "2024-01-01T12:00:00Z",
                "metrics": {
                    "battery_percentage": 85,
                    "temperature": 23.5,
                    "cpu_usage": 45.2
                }
            }
        ],
        "summary": {
            "total_points": 1440,
            "time_range": {
                "start": "2024-01-01T00:00:00Z",
                "end": "2024-01-01T23:59:59Z"
            },
            "aggregation": "5m"
        }
    }
}
```

### Get Device Analytics

**Endpoint**: `GET /devices/{device_id}/analytics`

**Description**: Get analytics and insights for device

**Query Parameters**:
- `period`: Analysis period (24h, 7d, 30d, 90d)
- `metrics`: Specific metrics to analyze

**Response**:
```json
{
    "success": true,
    "data": {
        "analytics": {
            "period": "24h",
            "battery": {
                "average_level": 78.5,
                "min_level": 45.0,
                "max_level": 100.0,
                "charging_cycles": 2,
                "estimated_life": "8.5 hours"
            },
            "performance": {
                "average_cpu": 35.2,
                "peak_cpu": 89.1,
                "average_memory": 65.8,
                "peak_memory": 87.3
            },
            "connectivity": {
                "wifi_uptime": 95.5,
                "cellular_uptime": 0.0,
                "lora_uptime": 98.2,
                "total_data_sent": 1048576,
                "total_data_received": 2097152
            },
            "location": {
                "total_distance": 15.2,
                "max_speed": 65.0,
                "locations_recorded": 288
            }
        }
    }
}
```

## Meshtastic Bridge API

### Send Mesh Message

**Endpoint**: `POST /mesh/messages`

**Description**: Send message to mesh network

**Request Body**:
```json
{
    "from_device": "t-deck-pro-001",
    "to_node": "node_id_456",
    "message_type": "text|position|telemetry|command",
    "payload": {
        "text": "Hello mesh network!",
        "channel": 0,
        "want_ack": true,
        "hop_limit": 3
    },
    "priority": "high|normal|low"
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "message_id": "msg_uuid",
        "status": "queued",
        "estimated_delivery": "2024-01-01T12:00:30Z"
    }
}
```

### Get Mesh Messages

**Endpoint**: `GET /devices/{device_id}/mesh/messages`

**Description**: Get mesh messages for device

**Query Parameters**:
- `channel`: Filter by channel
- `message_type`: Filter by message type
- `start_time`: Start timestamp
- `end_time`: End timestamp
- `limit`: Number of messages

**Response**:
```json
{
    "success": true,
    "data": {
        "messages": [
            {
                "id": "msg_uuid",
                "from_node": "node_id_123",
                "to_node": "node_id_456",
                "message_type": "text",
                "payload": {
                    "text": "Hello!",
                    "channel": 0
                },
                "timestamp": "2024-01-01T12:00:00Z",
                "hop_count": 2,
                "signal_strength": -75,
                "delivered": true
            }
        ]
    }
}
```

### Get Mesh Network Status

**Endpoint**: `GET /devices/{device_id}/mesh/network`

**Description**: Get mesh network topology and status

**Response**:
```json
{
    "success": true,
    "data": {
        "network": {
            "node_count": 15,
            "active_nodes": 12,
            "channels": [
                {
                    "id": 0,
                    "name": "Primary",
                    "frequency": 915.0,
                    "active_nodes": 12
                }
            ],
            "nodes": [
                {
                    "id": "node_id_123",
                    "name": "Base Station",
                    "last_seen": "2024-01-01T12:00:00Z",
                    "signal_strength": -65,
                    "hop_distance": 1,
                    "battery_level": 95,
                    "position": {
                        "latitude": 40.7128,
                        "longitude": -74.0060
                    }
                }
            ],
            "routes": [
                {
                    "from": "node_id_123",
                    "to": "node_id_456",
                    "hops": ["node_id_789"],
                    "quality": 85
                }
            ]
        }
    }
}
```

## User Management API

### Get User Profile

**Endpoint**: `GET /users/profile`

**Description**: Get current user profile

**Response**:
```json
{
    "success": true,
    "data": {
        "user": {
            "id": "user_uuid",
            "email": "user@example.com",
            "role": "admin|user",
            "profile": {
                "first_name": "John",
                "last_name": "Doe",
                "timezone": "America/New_York",
                "language": "en",
                "notifications": {
                    "email": true,
                    "push": false,
                    "sms": false
                }
            },
            "subscription": {
                "plan": "free|pro|enterprise",
                "expires_at": "2024-12-31T23:59:59Z",
                "device_limit": 10,
                "storage_limit": 1073741824
            },
            "created_at": "2024-01-01T12:00:00Z",
            "last_login": "2024-01-01T12:00:00Z"
        }
    }
}
```

### Update User Profile

**Endpoint**: `PUT /users/profile`

**Description**: Update user profile information

**Request Body**:
```json
{
    "profile": {
        "first_name": "John",
        "last_name": "Doe",
        "timezone": "America/New_York",
        "language": "en",
        "notifications": {
            "email": true,
            "push": false,
            "sms": false
        }
    }
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "profile_updated": true,
        "updated_at": "2024-01-01T12:00:00Z"
    }
}
```

### Change Password

**Endpoint**: `POST /users/change-password`

**Description**: Change user password

**Request Body**:
```json
{
    "current_password": "old_password",
    "new_password": "new_secure_password"
}
```

**Response**:
```json
{
    "success": true,
    "data": {
        "password_changed": true,
        "logout_other_sessions": true
    }
}
```

## System API

### Get System Status

**Endpoint**: `GET /system/status`

**Description**: Get overall system health and status

**Response**:
```json
{
    "success": true,
    "data": {
        "system": {
            "status": "healthy|degraded|down",
            "version": "1.0.0",
            "uptime": 86400,
            "services": {
                "api_server": "healthy",
                "ota_service": "healthy",
                "meshtastic_bridge": "healthy",
                "telemetry_collector": "healthy",
                "database": "healthy",
                "cache": "healthy"
            },
            "metrics": {
                "total_devices": 150,
                "active_devices": 142,
                "total_users": 25,
                "active_users": 18,
                "messages_processed": 15420,
                "data_transferred": 1073741824
            }
        }
    }
}
```

### Get System Metrics

**Endpoint**: `GET /system/metrics`

**Description**: Get detailed system performance metrics

**Response**:
```json
{
    "success": true,
    "data": {
        "metrics": {
            "api": {
                "requests_per_second": 45.2,
                "average_response_time": 125,
                "error_rate": 0.02
            },
            "database": {
                "connections": 15,
                "queries_per_second": 120,
                "average_query_time": 25
            },
            "cache": {
                "hit_rate": 0.95,
                "memory_usage": 67.8,
                "operations_per_second": 500
            },
            "storage": {
                "total_space": 1099511627776,
                "used_space": 549755813888,
                "available_space": 549755813888
            }
        }
    }