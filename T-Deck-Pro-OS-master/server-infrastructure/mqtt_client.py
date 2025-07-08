#!/usr/bin/env python3
"""
T-Deck-Pro MQTT Client for T-Deck-Pro OS Integration
Simple MQTT client implementation for device communication
"""

import json
import time
import hashlib
from datetime import datetime
from typing import Dict, Any, Optional, Callable
import paho.mqtt.client as mqtt

class TDeckProMQTTClient:
    """
    Lightweight MQTT client for T-Deck-Pro devices
    Handles all server communication via MQTT topics
    """
    
    def __init__(self, device_id: str, broker_host: str = "localhost", broker_port: int = 1883):
        self.device_id = device_id
        self.broker_host = broker_host
        self.broker_port = broker_port
        self.client = mqtt.Client()
        self.connected = False
        
        # Message handlers
        self.config_handler: Optional[Callable] = None
        self.ota_handler: Optional[Callable] = None
        self.app_handler: Optional[Callable] = None
        
        # Setup MQTT callbacks
        self.client.on_connect = self._on_connect
        self.client.on_disconnect = self._on_disconnect
        self.client.on_message = self._on_message
        
    def connect(self) -> bool:
        """Connect to MQTT broker"""
        try:
            self.client.connect(self.broker_host, self.broker_port, 60)
            self.client.loop_start()
            return True
        except Exception as e:
            print(f"MQTT connection failed: {e}")
            return False
            
    def disconnect(self):
        """Disconnect from MQTT broker"""
        self.client.loop_stop()
        self.client.disconnect()
        self.connected = False
        
    def _on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            self.connected = True
            print(f"Device {self.device_id} connected to MQTT broker")
            
            # Subscribe to device-specific topics
            client.subscribe(f"tdeckpro/{self.device_id}/config")
            client.subscribe(f"tdeckpro/{self.device_id}/ota")
            client.subscribe(f"tdeckpro/{self.device_id}/apps")
            
        else:
            print(f"MQTT connection failed with code {rc}")
            
    def _on_disconnect(self, client, userdata, rc):
        """MQTT disconnection callback"""
        self.connected = False
        print(f"Device {self.device_id} disconnected from MQTT broker")
        
    def _on_message(self, client, userdata, msg):
        """Handle incoming MQTT messages"""
        try:
            topic_parts = msg.topic.split('/')
            if len(topic_parts) < 3:
                return
                
            message_type = topic_parts[2]
            payload = json.loads(msg.payload.decode())
            
            if message_type == "config" and self.config_handler:
                self.config_handler(payload)
            elif message_type == "ota" and self.ota_handler:
                self.ota_handler(payload)
            elif message_type == "apps" and self.app_handler:
                self.app_handler(payload)
                
        except Exception as e:
            print(f"Error processing MQTT message: {e}")
            
    def register_device(self, device_info: Dict[str, Any]) -> bool:
        """Register device with server"""
        if not self.connected:
            return False
            
        topic = f"tdeckpro/{self.device_id}/register"
        payload = json.dumps(device_info)
        
        result = self.client.publish(topic, payload)
        return result.rc == mqtt.MQTT_ERR_SUCCESS
        
    def send_telemetry(self, telemetry_data: Dict[str, Any]) -> bool:
        """Send telemetry data to server"""
        if not self.connected:
            return False
            
        # Add timestamp
        telemetry_data['timestamp'] = datetime.now().isoformat()
        
        topic = f"tdeckpro/{self.device_id}/telemetry"
        payload = json.dumps(telemetry_data)
        
        result = self.client.publish(topic, payload)
        return result.rc == mqtt.MQTT_ERR_SUCCESS
        
    def send_status(self, status: str, additional_data: Optional[Dict[str, Any]] = None) -> bool:
        """Send status update to server"""
        if not self.connected:
            return False
            
        status_data = {
            'status': status,
            'timestamp': datetime.now().isoformat()
        }
        
        if additional_data:
            status_data.update(additional_data)
            
        topic = f"tdeckpro/{self.device_id}/status"
        payload = json.dumps(status_data)
        
        result = self.client.publish(topic, payload)
        return result.rc == mqtt.MQTT_ERR_SUCCESS
        
    def send_mesh_message(self, from_node: str, to_node: str, message_type: str, payload: Dict[str, Any]) -> bool:
        """Send mesh network message"""
        if not self.connected:
            return False
            
        mesh_data = {
            'from_node': from_node,
            'to_node': to_node,
            'message_type': message_type,
            'payload': payload,
            'timestamp': datetime.now().isoformat()
        }
        
        topic = f"tdeckpro/mesh/{message_type}"
        message = json.dumps(mesh_data)
        
        result = self.client.publish(topic, message)
        return result.rc == mqtt.MQTT_ERR_SUCCESS
        
    def set_config_handler(self, handler: Callable[[Dict[str, Any]], None]):
        """Set configuration message handler"""
        self.config_handler = handler
        
    def set_ota_handler(self, handler: Callable[[Dict[str, Any]], None]):
        """Set OTA update message handler"""
        self.ota_handler = handler
        
    def set_app_handler(self, handler: Callable[[Dict[str, Any]], None]):
        """Set app management message handler"""
        self.app_handler = handler
        
    def is_connected(self) -> bool:
        """Check if connected to MQTT broker"""
        return self.connected

# Example usage for T-Deck-Pro OS integration
class TDeckProServerIntegration:
    """
    Integration class for T-Deck-Pro OS
    Handles all server communication via MQTT
    """
    
    def __init__(self, device_id: str, broker_host: str = "localhost"):
        self.device_id = device_id
        self.mqtt_client = TDeckProMQTTClient(device_id, broker_host)
        
        # Set up message handlers
        self.mqtt_client.set_config_handler(self._handle_config)
        self.mqtt_client.set_ota_handler(self._handle_ota)
        self.mqtt_client.set_app_handler(self._handle_app)
        
        self.last_telemetry_time = 0
        self.telemetry_interval = 300  # 5 minutes
        
    def initialize(self) -> bool:
        """Initialize server connection"""
        if not self.mqtt_client.connect():
            return False
            
        # Register device
        device_info = {
            'device_type': 't-deck-pro',
            'firmware_version': '1.0.0',  # Get from system
            'hardware_version': '1.0',
            'capabilities': {
                'wifi': True,
                'lora': True,
                'cellular': True,
                'bluetooth': True,
                'gps': True,
                'eink_display': True
            },
            'config': {
                'timezone': 'UTC',
                'language': 'en'
            }
        }
        
        return self.mqtt_client.register_device(device_info)
        
    def update(self):
        """Call from main loop to handle periodic tasks"""
        current_time = time.time()
        
        # Send telemetry periodically
        if current_time - self.last_telemetry_time > self.telemetry_interval:
            self._send_telemetry()
            self.last_telemetry_time = current_time
            
    def _send_telemetry(self):
        """Send current device telemetry"""
        telemetry = {
            'battery_percentage': 85,  # Get from battery manager
            'temperature': 23.5,       # Get from sensors
            'cpu_usage': 45.2,         # Get from system
            'memory_usage': 67.8,      # Get from system
            'signal_strength': -65,    # Get from communication manager
            'gps_latitude': 40.7128,   # Get from GPS if available
            'gps_longitude': -74.0060,
            'wifi_connected': True,    # Get from WiFi manager
            'lora_active': True,       # Get from LoRa manager
            'cellular_connected': False, # Get from cellular manager
            'running_apps': ['meshtastic', 'file_manager']  # Get from app manager
        }
        
        self.mqtt_client.send_telemetry(telemetry)
        
    def _handle_config(self, config: Dict[str, Any]):
        """Handle configuration updates from server"""
        print(f"Received config update: {config}")
        
        # Apply configuration changes
        if 'update_interval' in config:
            self.telemetry_interval = config['update_interval']
            
        # Update system configuration
        # This would integrate with the existing config system
        
    def _handle_ota(self, ota_info: Dict[str, Any]):
        """Handle OTA update notifications"""
        print(f"OTA update available: {ota_info}")
        
        if ota_info.get('available'):
            # Download and install update
            # This would integrate with the existing OTA system
            pass
            
    def _handle_app(self, app_info: Dict[str, Any]):
        """Handle app management messages"""
        print(f"App management: {app_info}")
        
        # Handle app installation/removal
        # This would integrate with the existing app manager
        
    def send_mesh_message(self, to_node: str, message_type: str, payload: Dict[str, Any]):
        """Send message to mesh network via server"""
        return self.mqtt_client.send_mesh_message(
            self.device_id, to_node, message_type, payload
        )
        
    def is_connected(self) -> bool:
        """Check server connection status"""
        return self.mqtt_client.is_connected()
        
    def disconnect(self):
        """Disconnect from server"""
        self.mqtt_client.disconnect()

# C++ Integration Header for T-Deck-Pro OS
CPP_HEADER = '''
// server_mqtt_client.h
#pragma once

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "core/utils/logger.h"

class ServerMQTTClient {
private:
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    String deviceId;
    String brokerHost;
    int brokerPort;
    bool connected;
    
    unsigned long lastTelemetryTime;
    unsigned long telemetryInterval;
    
    // Message handlers
    void (*configHandler)(const JsonObject& config);
    void (*otaHandler)(const JsonObject& ota);
    void (*appHandler)(const JsonObject& app);
    
    void onMqttMessage(char* topic, byte* payload, unsigned int length);
    void sendTelemetry();
    
public:
    ServerMQTTClient(const String& deviceId, const String& brokerHost = "localhost", int brokerPort = 1883);
    
    bool initialize();
    void update();
    void disconnect();
    
    bool registerDevice(const JsonObject& deviceInfo);
    bool sendTelemetryData(const JsonObject& telemetry);
    bool sendStatus(const String& status, const JsonObject& additionalData = JsonObject());
    bool sendMeshMessage(const String& fromNode, const String& toNode, const String& messageType, const JsonObject& payload);
    
    void setConfigHandler(void (*handler)(const JsonObject& config));
    void setOtaHandler(void (*handler)(const JsonObject& ota));
    void setAppHandler(void (*handler)(const JsonObject& app));
    
    bool isConnected() const;
};
'''

if __name__ == "__main__":
    # Example usage
    integration = TDeckProServerIntegration("t-deck-pro-001")
    
    if integration.initialize():
        print("Server integration initialized")
        
        # Main loop simulation
        try:
            while True:
                integration.update()
                time.sleep(1)
        except KeyboardInterrupt:
            integration.disconnect()
            print("Disconnected from server")
    else:
        print("Failed to initialize server integration")