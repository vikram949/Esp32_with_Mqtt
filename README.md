**.

 Project Overview

This project turns an ESP32 into a Smart Bulb Controller that:

  Connects to WiFi using a Web Configuration Portal

  Saves WiFi credentials in non-volatile memory (Preferences)

  Connects to an MQTT Broker
  Controls LED using MQTT messages

  Supports Config Mode anytime via Serial or MQTT

  Features

✅ WiFi Auto Connect (Stored Credentials)
✅ Web-based WiFi Setup Portal (192.168.4.1)
✅ Async WiFi Network Scanning
✅ MQTT Control
✅ EEPROM/Preferences Storage
✅ Serial Debug Commands
✅ Auto Reconnect (WiFi + MQTT)

🛠 Hardware Required

ESP32 Development Board

Built-in LED (GPIO 2)

USB Cable

WiFi Network

MQTT Broker (e.g., Mosquitto)

  Libraries Used

WiFi.h

WebServer.h

EEPROM.h

PubSubClient.h

Preferences.h

Install PubSubClient from Arduino Library Manager.

⚙ How It Works
🔹 First Boot

If no WiFi credentials are saved:

ESP32 starts in Access Point Mode

Creates WiFi:

SSID: Smart_Bulb_Setup
Password: 12345678

Open browser:

http://192.168.4.1

Select WiFi network and enter password

Device saves credentials and restarts

🔹 Normal Mode

Connects to stored WiFi

Connects to MQTT broker

Subscribes to topic:

lights/LIGHT_<DeviceMAC>

Example:

lights/LIGHT_12345678
📡 MQTT Commands
Message	Action
1	LED ON
0	LED OFF
config	Start Config Mode
  Serial Commands

Open Serial Monitor (115200 baud):

Command	Action
config	Start WiFi setup
restart	Restart device
clear	Clear saved WiFi
status	Show WiFi & MQTT status
scan	Scan WiFi networks


Default AP password is 12345678

Change MQTT server in code:

const char* mqtt_server = "your_mqtt_server";
🎯 Use Cases

Smart Home Lighting

IoT Learning Project

MQTT Practice

Remote Device Control

  Author

Vikram Singh Rathore (Vicky)

IoT Developer | Embedded Systems Enthusiast

⭐ Support

If you like this project, give it a ⭐ on GitHub!**
