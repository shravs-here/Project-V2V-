
# Project V2V — Vehicle-to-Vehicle Emergency Network

A prototype Vehicle-to-Vehicle (V2V) accident response system built around ESP32 devices, GPS, ESP-NOW, Wi-Fi, and a Flask-based emergency server.

The system is designed to detect/report vehicle accidents, communicate vehicle information over ESP-NOW, obtain location information from GPS, and send emergency information to a remote server. The server stores accident reports in SQLite and provides a web-based emergency command center.

📌 Project Overview

The project has two main parts:

ESP32 vehicle-side system

Reads vehicle/sensor information.

Uses GPS to obtain latitude and longitude.

Uses ESP-NOW for vehicle-to-vehicle communication.

Uses Wi-Fi to send emergency alerts to the server.

Provides button handling for click/long-press events.

Emergency server

Built with Flask.

Receives accident alerts through an HTTP POST API.

Stores accident information in SQLite.

Provides accident statistics and history.

Provides a browser-based emergency dashboard.

Allows an accident to be acknowledged.

✨ Main Features

🚗 Vehicle-to-Vehicle communication using ESP-NOW

📍 GPS location acquisition and NMEA parsing

🌐 Wi-Fi connectivity using ESP32

🚨 Accident/emergency alert transmission

🔐 HTTPS communication between ESP32 and the server

🗄️ SQLite accident logging

📊 Emergency monitoring dashboard

🔔 New-accident notifications in the dashboard

✅ Accident acknowledgement

🔘 Button press, click, multi-click, and long-press handling

📡 Automatic ESP-NOW channel selection when configured with channel 0

🏗️ System Architecture

                    ┌──────────────────────────┐
                    │       VEHICLE ESP32      │
                    │                          │
                    │  MPU / Vehicle Sensors  │
                    │          │               │
                    │          ▼               │
                    │    VehicleSystem         │
                    │      /       \            │
                    │     ▼         ▼           │
                    │   GPS      ESP-NOW        │
                    │    │       Communication  │
                    │    │          │           │
                    │    └────┬─────┘           │
                    │         │                 │
                    │         ▼                 │
                    │       Wi-Fi               │
                    └─────────┬────────────────┘
                              │
                              │ HTTPS POST
                              ▼
                 ┌─────────────────────────────┐
                 │     Flask Emergency Server  │
                 │                             │
                 │  /accident                  │
                 │  /api/dashboard             │
                 │  /api/accident/.../acknowledge
                 │  /dashboard                 │
                 └─────────────┬───────────────┘
                               │
                               ▼
                       ┌───────────────┐
                       │ SQLite        │
                       │ accident_log  │
                       └───────────────┘
                               │
                               ▼
                 ┌─────────────────────────────┐
                 │ Emergency Command Center    │
                 │                             │
                 │ • Total accidents           │
                 │ • Today's accidents         │
                 │ • Latest incident           │
                 │ • Accident history          │
                 │ • Location                  │
                 │ • Severity                  │
                 │ • Acknowledge incident      │
                 └─────────────────────────────┘

📁 Project Structure

The main source files in this repository include:

Project-V2V-/
│
├── emergency_server.py
├── requirements.txt
│
├── esp1_sketch.ino
│
├── ESPNowDriver.cpp
├── ESPNowDriver.h
│
├── GPSDriver.cpp
├── GPSDriver.h
│
├── InternetDriver.cpp
│
├── button.cpp
├── button.h
│
└── ...

The ESP32 sketch also references additional project components such as:

MPUDriver
VehicleSystem

Those components are expected to be present in the complete Arduino project.

🖥️ Emergency Server

The server is implemented using Flask and stores accident information in a SQLite database.

The database is created automatically as:

accident_log.db

The accident table contains:

id
vehicle_id
accident_type
severity
latitude
longitude
timestamp
status

The server uses the Asia/Kolkata timezone for accident timestamps.

🌐 Server Routes

Home

GET /

Displays the V2V Emergency Network landing page.

Accident Receiver

POST /accident

Receives an accident report as JSON.

Example:

{
  "vehicle_id": "VEHICLE_01",
  "type": "ACCIDENT",
  "severity": "CRITICAL",
  "latitude": 16.494367,
  "longitude": 80.498942
}

The server stores the report and returns an accident ID.

Dashboard API

GET /api/dashboard

Returns:

Total accident count

Today's accident count

Latest accident

Accident history

Acknowledge Accident

POST /api/accident/<accident_id>/acknowledge

Changes an accident status to:

ACKNOWLEDGED

Emergency Dashboard

GET /dashboard

Opens the browser-based emergency command center.

The dashboard displays:

Total accidents

Accidents today

Server status

Latest accident

Vehicle ID

Accident type

Severity

Latitude

Longitude

Timestamp

Accident status

Accident history

Accident notifications

Location link

Acknowledge button

📍 GPS Module

GPSDriver communicates with a GPS module through ESP32 hardware serial.

Default configuration:

RX = GPIO 16
TX = GPIO 17
Baud rate = 9600

The driver processes:

GPRMC
GNRMC
GPGGA
GNGGA

NMEA sentences.

It provides three internal GPS states:

GPS_DEMO
GPS_REAL
GPS_NO_FIX

The driver also tracks whether the sensor is connected and whether a valid GPS fix is available.

If a GPS sensor timeout occurs, the driver falls back to the configured demo coordinates.

📡 ESP-NOW Communication

ESPNowDriver provides vehicle-to-vehicle communication using ESP-NOW.

The driver supports:

Broadcast messages

Peer registration

Peer discovery

Receive callbacks

Send status callbacks

Peer counting

Automatic channel selection

The default constructor uses:

ESPNowDriver espNow;

A channel value of 0 allows the driver to use the connected Wi-Fi channel.

The driver discovers new ESP-NOW peers and registers them dynamically.

🌐 Internet Communication

InternetDriver connects the ESP32 to a Wi-Fi network and sends emergency accident information to the server.

The accident payload contains:

{
  "vehicle_id": "...",
  "latitude": 0.0,
  "longitude": 0.0,
  "severity": "..."
}

The request is sent as an HTTPS POST with:

Content-Type: application/json
Accept: application/json

The driver also attempts to reconnect to Wi-Fi if the connection is lost.

Security note: The current prototype uses client.setInsecure() for HTTPS certificate handling. This disables certificate verification and should be replaced with proper certificate validation before production deployment.

🔘 Button Module

The Button class provides GPIO button handling.

Supported operations include:

isPressed()
wasPressed()
wasReleased()
wasClicked()
wasLongPressed()
getClickCount()

The implementation supports:

Press detection

Release detection

Short click detection

Multi-click counting

Long-press detection

The button uses:

INPUT_PULLUP

for its GPIO configuration.

🔧 Hardware / Software Requirements

Hardware

The uploaded project sources indicate the use of:

ESP32

GPS module

MPU/vehicle sensor module

Push button

Wi-Fi network

Exact sensor models and GPIO wiring for every component should be documented according to the final hardware setup.

Software

Server

Python 3

Flask

Gunicorn

SQLite

The current requirements.txt contains:

Flask
gunicorn

ESP32

The Arduino side requires:

Arduino IDE or another ESP32-compatible build environment

ESP32 Arduino support

ESP-NOW support

The project's custom driver/source files

🚀 Running the Emergency Server

1. Clone the repository

git clone https://github.com/shravs-here/Project-V2V-.git
cd Project-V2V-

2. Create a Python virtual environment

Windows:

python -m venv venv
venv\Scripts\activate

Linux/macOS:

python3 -m venv venv
source venv/bin/activate

3. Install dependencies

pip install -r requirements.txt

4. Start the Flask server

python emergency_server.py

The application initializes the SQLite database automatically.

Then open the dashboard in a browser:

http://127.0.0.1:5000/dashboard

The exact port depends on the Flask server configuration in the version of emergency_server.py being run.

Production / Gunicorn

The project includes Gunicorn as a dependency.

A typical deployment command is:

gunicorn emergency_server:app

🔌 ESP32 Setup

Open:

esp1_sketch.ino

in the Arduino IDE.

The main sketch initializes:

MPU
GPS
Internet
ESP-NOW
VehicleSystem

and then repeatedly calls:

vehicleSystem.update();

Important

Do not commit real Wi-Fi passwords or other credentials to GitHub.

Use placeholders or a separate local configuration file that is included in .gitignore.

For example:

const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";

🔄 Emergency Alert Flow

A typical alert flow is:

Accident / Emergency Event
          │
          ▼
     Vehicle ESP32
          │
          ├──────────────► GPS location
          │
          ├──────────────► ESP-NOW vehicle communication
          │
          ▼
       Wi-Fi
          │
          ▼
   HTTPS POST /accident
          │
          ▼
   Flask Emergency Server
          │
          ▼
      SQLite Database
          │
          ▼
   Emergency Dashboard
          │
          ▼
   Alert / Acknowledge

🧪 Testing

Test the server

Start the server:

python emergency_server.py

Open:

http://127.0.0.1:5000/

Then open:

http://127.0.0.1:5000/dashboard

Test the accident API

Example using curl:

curl -X POST http://127.0.0.1:5000/accident ^
  -H "Content-Type: application/json" ^
  -d "{\"vehicle_id\":\"TEST_VEHICLE\",\"type\":\"ACCIDENT\",\"severity\":\"CRITICAL\",\"latitude\":16.494367,\"longitude\":80.498942}"

On Linux/macOS:

curl -X POST http://127.0.0.1:5000/accident \
  -H "Content-Type: application/json" \
  -d '{"vehicle_id":"TEST_VEHICLE","type":"ACCIDENT","severity":"CRITICAL","latitude":16.494367,"longitude":80.498942}'

After a successful request, the accident should be stored in SQLite and become visible in the dashboard.

🔐 Security Considerations

This repository contains a prototype implementation. Before production deployment:

Never commit Wi-Fi passwords to GitHub.

Store secrets in environment variables or a local configuration file.

Replace WiFiClientSecure::setInsecure() with proper TLS certificate verification.

Add authentication/authorization to emergency APIs.

Validate and sanitize incoming accident data.

Use HTTPS for deployed server communication.

Protect the SQLite database and server environment.

Consider rate limiting and request authentication for the accident endpoint.

🚧 Current Prototype Limitations

The provided source code indicates that this is a prototype/demo system.

In particular:

GPS has a demo-coordinate fallback when a valid GPS fix is unavailable.

HTTPS certificate verification is disabled in the current ESP32 Internet driver.

Wi-Fi configuration is currently defined in the ESP32 sketch.

The complete project references additional modules such as MPUDriver and VehicleSystem; their source files should be included in the repository for a fully reproducible build.

Exact hardware wiring and sensor model information should be documented for the final hardware configuration.

🤝 Contributing

Fork the repository.

Create a feature branch:

git checkout -b feature/my-feature

Make your changes.

Commit:

git add .
git commit -m "Add my feature"

Push:

git push origin feature/my-feature

Open a Pull Request.

📜 License

No license file is currently included in the provided project files.

If this project is intended to be open source, add an appropriate license such as MIT, Apache-2.0, or GPL.

👨‍💻 Project

Project: Project V2V
Repository: Project-V2V-

Built as a prototype Vehicle-to-Vehicle emergency accident response network using ESP32, ESP-NOW, GPS, Wi-Fi, Flask, and SQLite.
