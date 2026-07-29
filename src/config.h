#pragma once
#include <Arduino.h>
#define DEBUG_SERIAL 1
static const char DEVICE_HOSTNAME[] = "smartfarm";
static const char AP_SSID[] = "SmartFarm_Setup";
static const uint32_t SESSION_TIMEOUT_MS = 30UL * 60UL * 1000UL;
static const uint8_t RELAY_COUNT = 4;
static const uint8_t RELAY_PINS[RELAY_COUNT] = {D1, D2, D5, D6}; // Boot-safe on NodeMCU; avoid D3/D4/D8
static const char *const RELAY_NAMES[RELAY_COUNT] = {"Pump", "Zone 1", "Sala Light", "Side Light"};
static const bool RELAY_ACTIVE_LOW = true;
static const uint16_t HTTP_PORT = 80;
static const uint16_t WS_PORT = 81;
static const uint32_t WS_PUSH_MS = 1000;
static const uint32_t SCHEDULER_TICK_MS = 1000;
