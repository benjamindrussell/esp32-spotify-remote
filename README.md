# ESP32 Spotify Remote

## Firmware for the Flipper Zero WiFi Devboard (ESP32) that talks to Spotify and lets you control playback 

### Installation
Currently the only option is to build from source
1. Clone the repo
2. Open in Arduino IDE
3. Go to preferences and add https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json to Additional Boards Manager Urls
<img width="1440" alt="image" src="https://github.com/benjamindrussell/esp32-spotify-remote/assets/46113261/1455b3ca-47db-4354-9a75-b7f6974ac836">
4. Go to boards manager and add esp32 by Espressif Systems, NOT Arduino ESP32 Boards by Arduino
<img width="1440" alt="image" src="https://github.com/benjamindrussell/esp32-spotify-remote/assets/46113261/44bebe45-c72e-43e8-bdea-74699c61f7dd">
5. Go to library manager and add ArduinoJson
<img width="1440" alt="image" src="https://github.com/benjamindrussell/esp32-spotify-remote/assets/46113261/a21e00d4-cf87-4b60-8d42-d32e3cbc9616">
6. Edit these 4 lines and add your client_id, client_secret, wifi ssid, and wifi password
<img width="1440" alt="image" src="https://github.com/benjamindrussell/esp32-spotify-remote/assets/46113261/0bcfab10-d21e-4d9e-b769-98807d284032">
7. For the official WiFi Devboard select ESP32S2 Dev Module and upload
<img width="1440" alt="image" src="https://github.com/benjamindrussell/esp32-spotify-remote/assets/46113261/9a38f6a7-e719-42a7-a8bf-a1c4077c9348">



