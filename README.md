# Smart Farm ESP8266 NodeMCU V3

Production-grade Smart Farm firmware for ESP8266 NodeMCU V3 using a clean, modular architecture. The project controls a four-channel active-LOW relay board, serves a mobile-first Bootstrap dashboard from LittleFS, supports WiFi STA with captive AP fallback, uses lightweight WebSocket telemetry, and persists users/settings/schedules across reboots.

## Hardware mapping

| Relay | Function | GPIO | NodeMCU pin | Boot safety |
| --- | --- | --- | --- | --- |
| 1 | Pump | GPIO5 | D1 | Safe |
| 2 | Zone 1 | GPIO4 | D2 | Safe |
| 3 | Sala Light | GPIO14 | D5 | Safe |
| 4 | Side Light | GPIO12 | D6 | Safe |

The firmware intentionally avoids D3/GPIO0, D4/GPIO2, and D8/GPIO15 for relay drive so relay hardware cannot break ESP8266 boot mode.

## Features

- WiFi STA mode with `SmartFarm_Setup` AP fallback and mDNS at `smartfarm.local`.
- Real-time dashboard: IP, RSSI, uptime, free heap, mode, and relay states.
- Manual relay control with session auth and CSRF headers.
- Emergency stop for privileged users.
- Safety interlock: pump can turn on only when Zone 1 is on; if all zones are off the pump is forced off.
- millis()-based scheduler with day masks, start minute, duration, and LittleFS persistence.
- User login with SHA256 password hash, session timeout, and route-level roles.
- Web OTA endpoint built on `Updater` with post-success reboot.
- Event/error logs stored in LittleFS and downloadable through authenticated endpoints.
- ESP8266 performance rules: no `delay()`, small JSON buffers, `F()` macro logging strings, and no RTOS.

## Project structure

```text
/src
  main.cpp
  config.h
  relay.cpp / relay.h
  scheduler.cpp / scheduler.h
  wifi.cpp / wifi.h
  webserver.cpp / webserver.h
  websocket.cpp / websocket.h
  auth.cpp / auth.h
  storage.cpp / storage.h
  ota.cpp / ota.h
  logger.cpp / logger.h
/data
  index.html
  login.html
  dashboard.html
  settings.html
  schedule.html
  users.html
  logs.html
  style.css
  script.js
platformio.ini
```

## Library dependencies

PlatformIO installs these automatically from `platformio.ini`:

- ArduinoJson 7
- WiFiManager 2
- arduinoWebSockets

The ESP8266 Arduino core provides ESP8266WiFi, ESP8266WebServer, LittleFS, ESP8266mDNS, ArduinoOTA, Updater, Hash, and EEPROM fallback support.

## Build instructions

```bash
pio run
```

For Arduino IDE, install ESP8266 board support and the dependencies above, then place the files in an Arduino sketch folder preserving the `src` and `data` layout.

## Flash instructions

```bash
pio run -t upload
pio run -t uploadfs
pio device monitor -b 115200
```

After first boot, connect to AP `SmartFarm_Setup` if no saved WiFi is available, configure WiFi, then open `http://smartfarm.local/` or the displayed IP address.

## Default login

- Username: `admin`
- Password: `admin123`

Change the default user immediately before production deployment by updating `/users.json` through your provisioning workflow.

## Usage guide

1. Log in from `/`.
2. Open `/dashboard.html`.
3. Turn Zone 1 on before turning Pump on.
4. Use Emergency Stop to immediately force all relays off.
5. Enable Auto mode through `/api/mode` once schedules have been provisioned in `/schedule.json`.
6. Download `/logs/event` for event logs.

## Extending relay channels

1. Increase `RELAY_COUNT` in `src/config.h`.
2. Add a boot-safe GPIO to `RELAY_PINS` and a label to `RELAY_NAMES`.
3. Expand `RelayController::states_` in `src/relay.h`.
4. Update dashboard labels in `data/script.js`.
5. Review `anyZoneOn()` if additional irrigation zones should permit pump operation.

## Verification checklist

- Compile with `pio run` and verify no warnings.
- Upload LittleFS with `pio run -t uploadfs`.
- Confirm ESP boots with all relays off.
- Confirm D3, D4, and D8 are not connected to relay inputs.
- Confirm WiFi portal appears when credentials are absent.
- Confirm mDNS resolves `smartfarm.local` on the LAN.
- Confirm all control APIs reject missing session/CSRF headers.
- Confirm Pump ON fails while Zone 1 is OFF.
- Confirm Pump turns OFF automatically when Zone 1 turns OFF.
- Confirm Emergency Stop turns all relays OFF.
- Confirm schedules still exist after reboot.
- Confirm OTA upload succeeds and the board reboots.
