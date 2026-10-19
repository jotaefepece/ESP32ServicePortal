# ESP32ServicePortal

Wi-Fi provisioning, automatic connection management, and web dashboard library for ESP32, based on ESP-IDF.

ESP32ServicePortal allows users to configure, monitor, and recover Wi-Fi connections through a web browser, without requiring serial commands.

## How it works

1. On startup, the ESP32 activates an access point (AP), providing access to the configuration dashboard.
2. The user can scan available Wi-Fi networks, select one, and enter its credentials.
3. The credentials are saved in non-volatile storage (NVS), allowing the device to reconnect after a restart or power loss.
4. Once the station (STA) connection is established, the configuration AP is automatically disabled after a short delay.
5. If the Wi-Fi connection is lost, the library attempts to reconnect and restores the AP when necessary, allowing the user to recover or change the configuration.

Previously saved credentials are reused automatically, avoiding repeated configuration after each restart.

## Features

- Wi-Fi configuration through a web browser.
- Automatic AP/STA management.
- Persistent Wi-Fi credentials using NVS.
- Automatic reconnection and configuration mode recovery.
- Wi-Fi network scanning and switching.
- Web dashboard for device status and log monitoring.
- Operation without serial commands.

## Compatibility

Tested successfully with:

- ESP32-WROVER
- ESP32-S3

Development and testing environment: Arduino IDE 1.8.19.

## Installation

### Arduino Library Manager (recommended)

1. Open Arduino IDE.
2. Go to **Sketch → Include Library → Manage Libraries**.
3. Search for `ESP32ServicePortal`.
4. Select the library and click **Install**.

### Manual installation

1. Download the library ZIP from [GitHub](https://github.com/jotaefepece/ESP32ServicePortal/releases).
2. Open Arduino IDE.
3. Select **Sketch → Include Library → Add .ZIP Library**.
4. Select the downloaded ZIP.

## Examples

Two examples are included:

- **PortalBasico:** Basic library initialization and periodic messages.
- **PortalFunciones:** Demonstrates the library's public functions.

Access them through **File → Examples → ESP32ServicePortal**.

## Default access

When the configuration AP is active:

- **Wi-Fi SSID:** `ESP32-Service-Portal`
- **Wi-Fi password:** `esp32service`
- **Dashboard:** `http://192.168.4.1`

Connect to the ESP32 access point and open the dashboard in a web browser to configure the Wi-Fi connection.

## Project background

ESP32ServicePortal originated from an ESP32-S3 firmware project developed using ESP-IDF as part of a Master's Final Project.

The Wi-Fi management, persistent storage, and web server components were adapted into an independent Arduino library, making these capabilities reusable in other ESP32 projects.

## Source code

https://github.com/jotaefepece/ESP32ServicePortal

## License

MIT License.

Copyright (c) 2026 Jose Poblete Cabezas.
