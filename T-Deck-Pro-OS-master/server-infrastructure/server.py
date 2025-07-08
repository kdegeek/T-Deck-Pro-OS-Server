#!/usr/bin/env python3
"""
T-Deck-Pro Lightweight Server
Simple MQTT-based server for T-Deck-Pro device management
"""

import asyncio
import json
import logging
import sqlite3
import time
from datetime import datetime
from pathlib import Path
from typing import Dict, Any, Optional

import paho.mqtt.client as mqtt
from fastapi import FastAPI, HTTPException, UploadFile, File
from fastapi.staticfiles import StaticFiles
from fastapi.responses import HTMLResponse, FileResponse
import uvicorn

# Configuration
MQTT_BROKER = "localhost"
MQTT_PORT = 1883
WEB_PORT = 8000
DB_PATH = "data/tdeckpro.db"
OTA_PATH = "data/ota-updates"
LOG_PATH = "data/logs"

# Setup logging
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(name)s - %(levelname)s - %(message)s',
    handlers=[
        logging.FileHandler(f"{LOG_PATH}/server.log"),
        logging.StreamHandler()
    ]
)
logger = logging.getLogger(__name__)

class TDeckProServer:
    def __init__(self):
        self.mqtt_client = None
        self.db_path = DB_PATH
        self.devices = {}  # In-memory device cache
        self.setup_directories()
        self.setup_database()
        
    def setup_directories(self):
        """Create necessary directories"""
        Path("data").mkdir(exist_ok=True)
        Path(OTA_PATH).mkdir(exist_ok=True)
        Path(LOG_PATH).mkdir(exist_ok=True)
        Path("static").mkdir(exist_ok=True)
        
    def setup_database(self):
        """Initialize SQLite database with simple schema"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Devices table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS devices (
                device_id TEXT PRIMARY KEY,
                device_type TEXT,
                firmware_version TEXT,
                last_seen TIMESTAMP,
                status TEXT DEFAULT 'offline',
                config TEXT,  -- JSON config
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Telemetry table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS telemetry (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                device_id TEXT,
                timestamp TIMESTAMP,
                data TEXT,  -- JSON telemetry data
                FOREIGN KEY (device_id) REFERENCES devices (device_id)
            )
        ''')
        
        # OTA updates table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS ota_updates (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                version TEXT,
                type TEXT,  -- 'firmware' or 'app'
                filename TEXT,
                checksum TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Apps table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS apps (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                app_id TEXT,
                name TEXT,
                version TEXT,
                filename TEXT,
                description TEXT,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        # Mesh messages table
        cursor.execute('''
            CREATE TABLE IF NOT EXISTS mesh_messages (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                from_node TEXT,
                to_node TEXT,
                message_type TEXT,
                payload TEXT,
                timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
        ''')
        
        conn.commit()
        conn.close()
        logger.info("Database initialized")
        
    def setup_mqtt(self):
        """Setup MQTT client and callbacks"""
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_message = self.on_mqtt_message
        
        try:
            self.mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
            self.mqtt_client.loop_start()
            logger.info(f"Connected to MQTT broker at {MQTT_BROKER}:{MQTT_PORT}")
        except Exception as e:
            logger.error(f"Failed to connect to MQTT broker: {e}")
            
    def on_mqtt_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            logger.info("MQTT connected successfully")
            # Subscribe to all T-Deck-Pro topics
            client.subscribe("tdeckpro/+/telemetry")
            client.subscribe("tdeckpro/+/status")
            client.subscribe("tdeckpro/+/register")
            client.subscribe("tdeckpro/mesh/+")
        else:
            logger.error(f"MQTT connection failed with code {rc}")
            
    def on_mqtt_message(self, client, userdata, msg):
        """Handle incoming MQTT messages"""
        try:
            topic_parts = msg.topic.split('/')
            if len(topic_parts) < 3:
                return
                
            if topic_parts[0] != "tdeckpro":
                return
                
            device_id = topic_parts[1]
            message_type = topic_parts[2]
            
            payload = json.loads(msg.payload.decode())
            
            if message_type == "register":
                self.handle_device_registration(device_id, payload)
            elif message_type == "telemetry":
                self.handle_telemetry(device_id, payload)
            elif message_type == "status":
                self.handle_status_update(device_id, payload)
            elif topic_parts[1] == "mesh":
                self.handle_mesh_message(topic_parts[2], payload)
                
        except Exception as e:
            logger.error(f"Error processing MQTT message: {e}")
            
    def handle_device_registration(self, device_id: str, data: Dict[str, Any]):
        """Handle device registration"""
        logger.info(f"Device registration: {device_id}")
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT OR REPLACE INTO devices 
            (device_id, device_type, firmware_version, last_seen, status, config)
            VALUES (?, ?, ?, ?, ?, ?)
        ''', (
            device_id,
            data.get('device_type', 'unknown'),
            data.get('firmware_version', '0.0.0'),
            datetime.now(),
            'online',
            json.dumps(data.get('config', {}))
        ))
        
        conn.commit()
        conn.close()
        
        # Send welcome configuration
        self.send_device_config(device_id)
        
    def handle_telemetry(self, device_id: str, data: Dict[str, Any]):
        """Handle device telemetry data"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Update device last seen
        cursor.execute('''
            UPDATE devices SET last_seen = ?, status = 'online' 
            WHERE device_id = ?
        ''', (datetime.now(), device_id))
        
        # Store telemetry
        cursor.execute('''
            INSERT INTO telemetry (device_id, timestamp, data)
            VALUES (?, ?, ?)
        ''', (device_id, datetime.now(), json.dumps(data)))
        
        conn.commit()
        conn.close()
        
        # Update in-memory cache
        self.devices[device_id] = {
            'last_seen': datetime.now(),
            'telemetry': data
        }
        
    def handle_status_update(self, device_id: str, data: Dict[str, Any]):
        """Handle device status updates"""
        logger.info(f"Status update from {device_id}: {data.get('status')}")
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            UPDATE devices SET status = ?, last_seen = ? 
            WHERE device_id = ?
        ''', (data.get('status', 'unknown'), datetime.now(), device_id))
        
        conn.commit()
        conn.close()
        
    def handle_mesh_message(self, message_type: str, data: Dict[str, Any]):
        """Handle mesh network messages"""
        logger.info(f"Mesh message: {message_type}")
        
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            INSERT INTO mesh_messages (from_node, to_node, message_type, payload)
            VALUES (?, ?, ?, ?)
        ''', (
            data.get('from_node'),
            data.get('to_node'),
            message_type,
            json.dumps(data.get('payload', {}))
        ))
        
        conn.commit()
        conn.close()
        
    def send_device_config(self, device_id: str):
        """Send configuration to device"""
        config = {
            'server_time': datetime.now().isoformat(),
            'update_interval': 300,  # 5 minutes
            'auto_update': True
        }
        
        topic = f"tdeckpro/{device_id}/config"
        self.mqtt_client.publish(topic, json.dumps(config))
        logger.info(f"Sent config to {device_id}")
        
    def check_for_updates(self, device_id: str, current_version: str) -> Optional[Dict[str, Any]]:
        """Check if updates are available for device"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT version, filename, checksum FROM ota_updates 
            WHERE type = 'firmware' AND version > ? 
            ORDER BY created_at DESC LIMIT 1
        ''', (current_version,))
        
        result = cursor.fetchone()
        conn.close()
        
        if result:
            return {
                'available': True,
                'version': result[0],
                'filename': result[1],
                'checksum': result[2],
                'download_url': f'/ota/download/{result[1]}'
            }
        return {'available': False}
        
    def get_device_list(self) -> list:
        """Get list of all devices"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT device_id, device_type, firmware_version, last_seen, status
            FROM devices ORDER BY last_seen DESC
        ''')
        
        devices = []
        for row in cursor.fetchall():
            devices.append({
                'device_id': row[0],
                'device_type': row[1],
                'firmware_version': row[2],
                'last_seen': row[3],
                'status': row[4]
            })
            
        conn.close()
        return devices
        
    def get_device_telemetry(self, device_id: str, limit: int = 100) -> list:
        """Get recent telemetry for device"""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        cursor.execute('''
            SELECT timestamp, data FROM telemetry 
            WHERE device_id = ? 
            ORDER BY timestamp DESC LIMIT ?
        ''', (device_id, limit))
        
        telemetry = []
        for row in cursor.fetchall():
            telemetry.append({
                'timestamp': row[0],
                'data': json.loads(row[1])
            })
            
        conn.close()
        return telemetry

# Global server instance
server = TDeckProServer()

# FastAPI web interface
app = FastAPI(title="T-Deck-Pro Server", description="Lightweight T-Deck-Pro management server")

@app.on_event("startup")
async def startup_event():
    """Initialize server on startup"""
    server.setup_mqtt()

@app.get("/", response_class=HTMLResponse)
async def dashboard():
    """Simple web dashboard"""
    devices = server.get_device_list()
    
    html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>T-Deck-Pro Server</title>
        <style>
            body {{ font-family: Arial, sans-serif; margin: 20px; }}
            table {{ border-collapse: collapse; width: 100%; }}
            th, td {{ border: 1px solid #ddd; padding: 8px; text-align: left; }}
            th {{ background-color: #f2f2f2; }}
            .online {{ color: green; }}
            .offline {{ color: red; }}
        </style>
    </head>
    <body>
        <h1>T-Deck-Pro Server Dashboard</h1>
        <h2>Connected Devices ({len(devices)})</h2>
        <table>
            <tr>
                <th>Device ID</th>
                <th>Type</th>
                <th>Firmware</th>
                <th>Last Seen</th>
                <th>Status</th>
            </tr>
    """
    
    for device in devices:
        status_class = "online" if device['status'] == 'online' else "offline"
        html += f"""
            <tr>
                <td><a href="/device/{device['device_id']}">{device['device_id']}</a></td>
                <td>{device['device_type']}</td>
                <td>{device['firmware_version']}</td>
                <td>{device['last_seen']}</td>
                <td class="{status_class}">{device['status']}</td>
            </tr>
        """
    
    html += """
        </table>
        <br>
        <a href="/upload">Upload OTA Update</a> | 
        <a href="/mesh">Mesh Messages</a> |
        <a href="/logs">View Logs</a>
    </body>
    </html>
    """
    return html

@app.get("/device/{device_id}")
async def device_details(device_id: str):
    """Device details page"""
    telemetry = server.get_device_telemetry(device_id, 10)
    
    html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Device: {device_id}</title>
        <style>
            body {{ font-family: Arial, sans-serif; margin: 20px; }}
            .telemetry {{ background: #f5f5f5; padding: 10px; margin: 5px 0; }}
        </style>
    </head>
    <body>
        <h1>Device: {device_id}</h1>
        <a href="/">← Back to Dashboard</a>
        
        <h2>Recent Telemetry</h2>
    """
    
    for entry in telemetry:
        html += f"""
        <div class="telemetry">
            <strong>{entry['timestamp']}</strong><br>
            <pre>{json.dumps(entry['data'], indent=2)}</pre>
        </div>
        """
    
    html += """
    </body>
    </html>
    """
    return HTMLResponse(html)

@app.get("/api/devices")
async def api_devices():
    """API endpoint for device list"""
    return server.get_device_list()

@app.get("/api/device/{device_id}/telemetry")
async def api_device_telemetry(device_id: str, limit: int = 100):
    """API endpoint for device telemetry"""
    return server.get_device_telemetry(device_id, limit)

@app.get("/api/device/{device_id}/updates")
async def api_check_updates(device_id: str, current_version: str):
    """API endpoint to check for updates"""
    return server.check_for_updates(device_id, current_version)

@app.post("/api/ota/upload")
async def upload_ota(file: UploadFile = File(...), version: str = "", type: str = "firmware"):
    """Upload OTA update file"""
    if not file.filename:
        raise HTTPException(status_code=400, detail="No file provided")
    
    # Save file
    file_path = Path(OTA_PATH) / file.filename
    with open(file_path, "wb") as f:
        content = await file.read()
        f.write(content)
    
    # Calculate checksum (simple)
    import hashlib
    checksum = hashlib.sha256(content).hexdigest()
    
    # Store in database
    conn = sqlite3.connect(server.db_path)
    cursor = conn.cursor()
    
    cursor.execute('''
        INSERT INTO ota_updates (version, type, filename, checksum)
        VALUES (?, ?, ?, ?)
    ''', (version, type, file.filename, checksum))
    
    conn.commit()
    conn.close()
    
    logger.info(f"OTA update uploaded: {file.filename} v{version}")
    return {"message": "Upload successful", "filename": file.filename, "checksum": checksum}

@app.get("/ota/download/{filename}")
async def download_ota(filename: str):
    """Download OTA update file"""
    file_path = Path(OTA_PATH) / filename
    if not file_path.exists():
        raise HTTPException(status_code=404, detail="File not found")
    
    return FileResponse(file_path, filename=filename)

if __name__ == "__main__":
    logger.info("Starting T-Deck-Pro Server")
    uvicorn.run(app, host="0.0.0.0", port=WEB_PORT)