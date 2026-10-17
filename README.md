# ESP32ServicePortal

Wi-Fi provisioning and web dashboard library for ESP32, based on ESP-IDF.

ESP32ServicePortal allows Wi-Fi configuration through a web browser, without requiring serial commands.

## Features

- Wi-Fi configuration through an access point and web dashboard.
- Station (STA) connection to an existing Wi-Fi network.
- Persistent Wi-Fi credentials using NVS.
- Automatic reconnection and configuration mode recovery.
- Web-based serial log monitor.
- Wi-Fi network scanning and switching.

## Compatibility

Tested successfully with:

- ESP32-WROVER
- ESP32-S3

Arduino IDE 1.8.19.

## Installation

1. Download the library ZIP from GitHub.
2. Open Arduino IDE.
3. Select **Sketch → Include Library → Add .ZIP Library**.
4. Select the downloaded ZIP.
5. Open **File → Examples → ESP32ServicePortal**.

## Examples

- **PortalBasico:** Basic initialization and periodic messages.
- **PortalFunciones:** Demonstrates the library's public functions.

## Default access

- Wi-Fi SSID: `ESP32-Service-Portal`
- Wi-Fi password: `esp32service`
- Dashboard: `http://192.168.4.1`

## License

MIT License.

Copyright (c) 2026 Jose Poblete Cabezas.
