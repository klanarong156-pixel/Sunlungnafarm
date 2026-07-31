# Smart Farm V7

Smart Farm V7 is a production-oriented ESP8266 smart farm controller for NodeMCU-class boards. It preserves the original RTC + MQTT + dashboard behavior and upgrades the project with four independent active-LOW relay channels, LittleFS JSON storage, schedule automation, sensor telemetry, OTA update support, structured logs, and a redesigned responsive dashboard.

## Firmware capabilities

- **Four independent relays**
  - Relay 1: Water Pump
  - Relay 2: Zone 1 Irrigation
  - Relay 3: House Light
  - Relay 4: Pavilion Light
  - Every relay has its own GPIO, state, MQTT set topic, MQTT state topic, dashboard button, safe boot-off state, active-LOW output, persisted state, and manual override handling.
- **Reusable relay API**: `setRelay()`, `toggleRelay()`, `publishRelayState()`, `loadRelayState()`, and `saveRelayState()`.
- **Schedule system**
  - Schedules are stored in `/schedules.json` on LittleFS.
  - Each schedule supports enabled/disabled state, relay number, name, ON time, OFF time or duration, selected day mask, repeat, and automatic execution.
  - The JSON schema supports add, edit, delete, duplicate, sort, search, grouped display, schedule count, and next-execution presentation from the dashboard layer.
  - The firmware validates corrupted JSON by recreating invalid files and prevents duplicate execution per schedule/minute.
  - Multiple schedules can run simultaneously on different relays.
- **Manual/Auto logic**
  - Default visible mode is Manual.
  - A relay enters `Scheduled Auto Running` only while a schedule is executing.
  - Manual commands cancel the current auto task and immediately return that relay to Manual.
- **MQTT integration**
  - Base topic: `smartfarm`.
  - Commands: `smartfarm/relay1/set` ... `smartfarm/relay4/set`.
  - States: `smartfarm/relay1/state` ... `smartfarm/relay4/state`.
  - System topics: `smartfarm/status`, `smartfarm/heartbeat`, `smartfarm/sensors`, `smartfarm/schedule`, `smartfarm/schedule/set`, `smartfarm/firmware`, and `smartfarm/telegram/notify`.
  - Retained state publishing, Last Will (`OFFLINE`), heartbeat, and reconnect are enabled.
- **OTA**
  - Browser OTA endpoint is available at `/update` on the ESP8266 web server.
  - The dashboard exposes OTA status and the firmware reboots after ESP8266 HTTP update success.
- **Telegram hooks**
  - Relay changes, schedule start/finish, WiFi/MQTT events, restarts, daily summary, OTA status, and sensor alarms are logged and published to `smartfarm/telegram/notify` for a Telegram bridge/bot service.
- **RTC and sensors**
  - DS3231 support with lost-power recovery.
  - DHT11/DHT22 architecture via `DHT_TYPE`.
  - Soil moisture, water level, and rain sensor inputs are published to MQTT and dashboard cards.
- **Storage**
  - LittleFS stores settings, relay states, schedules, calibration data, and CSV history.
  - Missing files are recreated automatically.
- **Stability**
  - Non-blocking `millis()` scheduling, watchdog feed, WiFi/MQTT reconnect, boot-safe relay output, heap heartbeat, CPU-friendly `yield()`, and no runtime `delay()` calls in the main loop.

## GPIO map

| Relay | Function | GPIO constant | NodeMCU pin | Active state |
|---|---|---|---|---|
| 1 | Water Pump | `D5` | GPIO14 | LOW |
| 2 | Zone 1 Irrigation | `D6` | GPIO12 | LOW |
| 3 | House Light | `D7` | GPIO13 | LOW |
| 4 | Pavilion Light | `D8` | GPIO15 | LOW |

> Note: `D8/GPIO15` must be LOW during boot on many ESP8266 boards. Use a relay board or transistor stage that preserves correct boot strapping.

## Dashboard

The updated `index.html` dashboard includes:

- WiFi, MQTT, RTC, Telegram, OTA, current time, uptime, and heap indicators.
- Temperature, humidity, soil moisture, water level, rain, and chart sections.
- Four relay cards with ON/OFF, mode, running timer, last action, and manual buttons.
- Schedule search, count, add, duplicate, enable/disable, delete confirmation, and retained MQTT publishing.
- Dark mode, light mode, toast notifications, responsive mobile layout, PWA manifest, and offline-capable app shell.
- Admin/user records in local storage as a dashboard-side convenience. Production deployments should replace this with server-side authentication or ESP-hosted hashed credential checks.

## Required Arduino libraries

Install these libraries in Arduino IDE or PlatformIO:

- ESP8266 board package
- WiFiManager
- PubSubClient
- ArduinoJson
- RTClib
- DHT sensor library
- LittleFS support for ESP8266

## Build and upload

1. Open `Smartfarm_Refactored.ino` in Arduino IDE.
2. Select an ESP8266 board such as NodeMCU 1.0.
3. Install the required libraries listed above.
4. Upload the firmware.
5. Upload LittleFS data if you add custom JSON files, or let the firmware create defaults at first boot.
6. Connect to the setup AP `SmartFarmV7_Setup` if WiFi is not configured.
7. Browse to the ESP8266 IP address for status endpoints and OTA at `/update`.
8. Serve or open `index.html` for the dashboard and connect it to MQTT over WebSockets.

## MQTT topic reference

| Topic | Direction | Payload |
|---|---|---|
| `smartfarm/relay1/set` | Subscribe | `ON`, `OFF`, or `TOGGLE` |
| `smartfarm/relay1/state` | Publish retained | JSON relay state/mode/timer/last action |
| `smartfarm/relay2/set` | Subscribe | `ON`, `OFF`, or `TOGGLE` |
| `smartfarm/relay2/state` | Publish retained | JSON relay state/mode/timer/last action |
| `smartfarm/relay3/set` | Subscribe | `ON`, `OFF`, or `TOGGLE` |
| `smartfarm/relay3/state` | Publish retained | JSON relay state/mode/timer/last action |
| `smartfarm/relay4/set` | Subscribe | `ON`, `OFF`, or `TOGGLE` |
| `smartfarm/relay4/state` | Publish retained | JSON relay state/mode/timer/last action |
| `smartfarm/status` | Publish retained | `ONLINE` / `OFFLINE` |
| `smartfarm/heartbeat` | Publish | JSON uptime, heap, WiFi, RTC, version |
| `smartfarm/sensors` | Publish retained | JSON sensor telemetry |
| `smartfarm/schedule` | Publish retained | JSON schedules |
| `smartfarm/schedule/set` | Subscribe | JSON schedules update |
| `smartfarm/restart` | Subscribe | Any payload restarts ESP |

## Schedule JSON example

```json
{
  "schedules": [
    {
      "id": 1001,
      "enabled": true,
      "relay": 1,
      "name": "Morning water",
      "onTime": "06:00",
      "offTime": "06:10",
      "durationMin": 10,
      "daysMask": 0,
      "repeat": true,
      "autoExecution": true
    }
  ]
}
```

`daysMask` uses bit 0 for Sunday through bit 6 for Saturday. A value of `0` means every day.

## Version

Current project name: **Smart Farm V7**
Firmware version: **7.0.0**
