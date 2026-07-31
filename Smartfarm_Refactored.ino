#include <Arduino.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>
#include <LittleFS.h>
#include <PubSubClient.h>
#include <RTClib.h>
#include <WiFiClientSecure.h>
#include <WiFiManager.h>
#include <Wire.h>

// =============================================================
// Smart Farm V7 - ESP8266 production firmware
// =============================================================
#define FW_NAME "Smart Farm V7"
#define FW_VERSION "7.0.0"

// ---------- Hardware mapping (Active LOW relays) ----------
const uint8_t RELAY_COUNT = 4;
const uint8_t RELAY_PINS[RELAY_COUNT] = {D5, D6, D7, D8};
const char RELAY_NAMES[RELAY_COUNT][18] = {
  "Water Pump", "Zone 1 Irrigation", "House Light", "Pavilion Light"
};
const bool RELAY_ACTIVE_LOW = true;

const uint8_t I2C_SDA = D2;
const uint8_t I2C_SCL = D1;
const uint8_t DHT_PIN = D4;
const uint8_t DHT_TYPE = DHT22;  // Change to DHT11 when required.
const uint8_t SOIL_PIN = A0;
const uint8_t WATER_LEVEL_PIN = D0;
const uint8_t RAIN_PIN = D3;

// ---------- MQTT defaults ----------
const char DEFAULT_MQTT_HOST[] = "650188a0ee2b4367b7c131fb385590a9.s1.eu.hivemq.cloud";
const uint16_t DEFAULT_MQTT_PORT = 8883;
const char DEFAULT_MQTT_USER[] = "smartfarm";
const char DEFAULT_MQTT_PASS[] = "Kla12345";
const char MQTT_BASE[] = "smartfarm";

// ---------- Files ----------
const char FILE_SETTINGS[] = "/settings.json";
const char FILE_RELAYS[] = "/relays.json";
const char FILE_SCHEDULES[] = "/schedules.json";
const char FILE_CALIBRATION[] = "/calibration.json";
const char FILE_HISTORY[] = "/history.csv";

// ---------- Timing ----------
const unsigned long MQTT_RECONNECT_MS = 5000;
const unsigned long HEARTBEAT_MS = 30000;
const unsigned long SENSOR_MS = 10000;
const unsigned long RTC_MS = 1000;
const unsigned long WIFI_CHECK_MS = 10000;
const unsigned long SUMMARY_MS = 86400000UL;
const unsigned long AUTO_RECOVERY_MS = 60000;

struct Settings {
  char mqttHost[80];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPass[48];
  char telegramToken[96];
  char telegramChatId[32];
  char adminHash[65];
  char userHash[65];
};

struct RelayRuntime {
  bool state;
  bool autoRunning;
  uint32_t autoScheduleId;
  unsigned long autoStartedMs;
  unsigned long lastChangeMs;
  char lastAction[40];
};

struct ScheduleItem {
  uint32_t id;
  bool enabled;
  uint8_t relay;
  char name[32];
  char onTime[6];
  char offTime[6];
  uint16_t durationMin;
  uint8_t daysMask;  // bit0 Sunday ... bit6 Saturday, 0 = every day
  bool repeat;
  bool autoExecution;
  uint16_t lastOnKey;
  uint16_t lastOffKey;
};

const uint8_t MAX_SCHEDULES = 24;  // ESP8266-safe cap; JSON schema supports growing this value.
Settings settings;
RelayRuntime relays[RELAY_COUNT];
ScheduleItem schedules[MAX_SCHEDULES];
uint8_t scheduleCount = 0;
bool rtcOk = false;
bool mqttWasConnected = false;
bool wifiWasConnected = false;
float temperature = NAN;
float humidity = NAN;
uint16_t soilRaw = 0;
bool waterLevelOk = true;
bool rainDetected = false;
unsigned long lastMqttReconnect = 0;
unsigned long lastHeartbeat = 0;
unsigned long lastSensorRead = 0;
unsigned long lastRtcTick = 0;
unsigned long lastWifiCheck = 0;
unsigned long lastSummary = 0;

RTC_DS3231 rtc;
DHT dht(DHT_PIN, DHT_TYPE);
WiFiClientSecure tlsClient;
PubSubClient mqtt(tlsClient);
ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;

void safeCopy(char *dst, size_t size, const char *src) {
  if (size == 0) return;
  strncpy(dst, src ? src : "", size - 1);
  dst[size - 1] = '\0';
}

void logEvent(const char *category, const char *message) {
  char line[140];
  snprintf(line, sizeof(line), "{\"ms\":%lu,\"cat\":\"%s\",\"msg\":\"%s\"}", millis(), category, message);
  Serial.println(line);
}

void ensureFile(const char *path, const char *content) {
  if (LittleFS.exists(path)) return;
  File f = LittleFS.open(path, "w");
  if (f) { f.print(content); f.close(); }
}

bool readJson(const char *path, JsonDocument &doc) {
  File f = LittleFS.open(path, "r");
  if (!f) return false;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    char msg[80];
    snprintf(msg, sizeof(msg), "Corrupt JSON recreated: %s", path);
    logEvent("storage", msg);
    LittleFS.remove(path);
    return false;
  }
  return true;
}

bool writeJson(const char *path, JsonDocument &doc) {
  File f = LittleFS.open(path, "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

void publishTopic(const char *suffix, const char *payload, bool retained = true) {
  if (!mqtt.connected()) return;
  char topic[96];
  snprintf(topic, sizeof(topic), "%s/%s", MQTT_BASE, suffix);
  mqtt.publish(topic, payload, retained);
}

void notifyTelegram(const char *message) {
  // Non-blocking placeholder: publish notifications to MQTT for a bridge/bot service.
  publishTopic("telegram/notify", message, false);
  logEvent("telegram", message);
}

void publishRelayState(uint8_t relayIndex) {
  if (relayIndex >= RELAY_COUNT) return;
  char suffix[32];
  char payload[160];
  snprintf(suffix, sizeof(suffix), "relay%u/state", relayIndex + 1);
  snprintf(payload, sizeof(payload), "{\"state\":\"%s\",\"mode\":\"%s\",\"lastAction\":\"%s\",\"timer\":%lu}",
           relays[relayIndex].state ? "ON" : "OFF",
           relays[relayIndex].autoRunning ? "Scheduled Auto Running" : "Manual",
           relays[relayIndex].lastAction,
           relays[relayIndex].autoRunning ? (millis() - relays[relayIndex].autoStartedMs) / 1000 : 0);
  publishTopic(suffix, payload, true);
}

void saveRelayState() {
  StaticJsonDocument<384> doc;
  JsonArray arr = doc.createNestedArray("relays");
  for (uint8_t i = 0; i < RELAY_COUNT; i++) arr.add(relays[i].state);
  writeJson(FILE_RELAYS, doc);
}

void loadRelayState() {
  StaticJsonDocument<384> doc;
  if (!readJson(FILE_RELAYS, doc)) return;
  JsonArray arr = doc["relays"].as<JsonArray>();
  for (uint8_t i = 0; i < RELAY_COUNT; i++) relays[i].state = arr[i] | false;
}

void setRelay(uint8_t relayIndex, bool state, const char *reason = "manual") {
  if (relayIndex >= RELAY_COUNT) return;
  if (relays[relayIndex].autoRunning && strcmp(reason, "schedule") != 0) {
    relays[relayIndex].autoRunning = false;
    relays[relayIndex].autoScheduleId = 0;
    notifyTelegram("Manual override canceled schedule");
  }
  relays[relayIndex].state = state;
  relays[relayIndex].lastChangeMs = millis();
  safeCopy(relays[relayIndex].lastAction, sizeof(relays[relayIndex].lastAction), reason);
  digitalWrite(RELAY_PINS[relayIndex], RELAY_ACTIVE_LOW ? !state : state);
  saveRelayState();
  publishRelayState(relayIndex);
  char msg[96];
  snprintf(msg, sizeof(msg), "%s %s (%s)", RELAY_NAMES[relayIndex], state ? "ON" : "OFF", reason);
  logEvent("relay", msg);
  notifyTelegram(msg);
}

void toggleRelay(uint8_t relayIndex, const char *reason = "manual") {
  if (relayIndex < RELAY_COUNT) setRelay(relayIndex, !relays[relayIndex].state, reason);
}

void loadSettings() {
  safeCopy(settings.mqttHost, sizeof(settings.mqttHost), DEFAULT_MQTT_HOST);
  settings.mqttPort = DEFAULT_MQTT_PORT;
  safeCopy(settings.mqttUser, sizeof(settings.mqttUser), DEFAULT_MQTT_USER);
  safeCopy(settings.mqttPass, sizeof(settings.mqttPass), DEFAULT_MQTT_PASS);
  safeCopy(settings.adminHash, sizeof(settings.adminHash), "03ac674216f3e15c761ee1a5e255f067953623c8b388b4459e13f978d7c846f4");
  safeCopy(settings.userHash, sizeof(settings.userHash), "03ac674216f3e15c761ee1a5e255f067953623c8b388b4459e13f978d7c846f4");
  StaticJsonDocument<768> doc;
  if (!readJson(FILE_SETTINGS, doc)) return;
  safeCopy(settings.mqttHost, sizeof(settings.mqttHost), doc["mqttHost"] | settings.mqttHost);
  settings.mqttPort = doc["mqttPort"] | settings.mqttPort;
  safeCopy(settings.mqttUser, sizeof(settings.mqttUser), doc["mqttUser"] | settings.mqttUser);
  safeCopy(settings.mqttPass, sizeof(settings.mqttPass), doc["mqttPass"] | settings.mqttPass);
  safeCopy(settings.telegramToken, sizeof(settings.telegramToken), doc["telegramToken"] | "");
  safeCopy(settings.telegramChatId, sizeof(settings.telegramChatId), doc["telegramChatId"] | "");
}

void loadSchedules() {
  scheduleCount = 0;
  DynamicJsonDocument doc(4096);
  if (!readJson(FILE_SCHEDULES, doc)) return;
  JsonArray arr = doc["schedules"].as<JsonArray>();
  for (JsonObject o : arr) {
    if (scheduleCount >= MAX_SCHEDULES) break;
    ScheduleItem &s = schedules[scheduleCount++];
    s.id = o["id"] | (1000 + scheduleCount);
    s.enabled = o["enabled"] | true;
    s.relay = constrain((int)(o["relay"] | 1), 1, 4) - 1;
    safeCopy(s.name, sizeof(s.name), o["name"] | "Schedule");
    safeCopy(s.onTime, sizeof(s.onTime), o["onTime"] | "06:00");
    safeCopy(s.offTime, sizeof(s.offTime), o["offTime"] | "");
    s.durationMin = o["durationMin"] | 10;
    s.daysMask = o["daysMask"] | 0;
    s.repeat = o["repeat"] | true;
    s.autoExecution = o["autoExecution"] | true;
    s.lastOnKey = 65535;
    s.lastOffKey = 65535;
  }
}

void publishSchedules() {
  DynamicJsonDocument doc(4096);
  JsonArray arr = doc.createNestedArray("schedules");
  for (uint8_t i = 0; i < scheduleCount; i++) {
    JsonObject o = arr.createNestedObject();
    o["id"] = schedules[i].id; o["enabled"] = schedules[i].enabled; o["relay"] = schedules[i].relay + 1;
    o["name"] = schedules[i].name; o["onTime"] = schedules[i].onTime; o["offTime"] = schedules[i].offTime;
    o["durationMin"] = schedules[i].durationMin; o["daysMask"] = schedules[i].daysMask; o["repeat"] = schedules[i].repeat;
    o["autoExecution"] = schedules[i].autoExecution;
  }
  char payload[2048];
  serializeJson(doc, payload, sizeof(payload));
  publishTopic("schedule", payload, true);
}

uint16_t minuteOfDay(const char *hhmm) {
  if (!hhmm || strlen(hhmm) < 5 || hhmm[2] != ':') return 65535;
  return atoi(hhmm) * 60 + atoi(hhmm + 3);
}

void checkSchedules() {
  if (!rtcOk) return;
  DateTime now = rtc.now();
  uint16_t minute = now.hour() * 60 + now.minute();
  uint16_t dayKey = (now.dayOfTheWeek() * 1440) + minute;
  for (uint8_t i = 0; i < scheduleCount; i++) {
    ScheduleItem &s = schedules[i];
    if (!s.enabled || !s.autoExecution) continue;
    if (s.daysMask && !(s.daysMask & (1 << now.dayOfTheWeek()))) continue;
    uint16_t onMin = minuteOfDay(s.onTime);
    uint16_t offMin = strlen(s.offTime) == 5 ? minuteOfDay(s.offTime) : (onMin + s.durationMin) % 1440;
    if (minute == onMin && s.lastOnKey != dayKey) {
      s.lastOnKey = dayKey;
      relays[s.relay].autoRunning = true;
      relays[s.relay].autoScheduleId = s.id;
      relays[s.relay].autoStartedMs = millis();
      setRelay(s.relay, true, "schedule");
      notifyTelegram("Schedule Started");
    }
    if (minute == offMin && s.lastOffKey != dayKey && relays[s.relay].autoScheduleId == s.id) {
      s.lastOffKey = dayKey;
      setRelay(s.relay, false, "schedule");
      relays[s.relay].autoRunning = false;
      relays[s.relay].autoScheduleId = 0;
      notifyTelegram("Schedule Finished");
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  char msg[512];
  size_t n = min((size_t)length, sizeof(msg) - 1);
  memcpy(msg, payload, n); msg[n] = '\0';
  logEvent("mqtt", topic);
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    char setTopic[40]; snprintf(setTopic, sizeof(setTopic), "%s/relay%u/set", MQTT_BASE, i + 1);
    if (strcmp(topic, setTopic) == 0) {
      if (strcasecmp(msg, "TOGGLE") == 0) toggleRelay(i);
      else setRelay(i, strcasecmp(msg, "ON") == 0 || strcmp(msg, "1") == 0);
      return;
    }
  }
  if (strcmp(topic, "smartfarm/schedule/set") == 0) {
    File f = LittleFS.open(FILE_SCHEDULES, "w");
    if (f) { f.write((const uint8_t*)msg, n); f.close(); loadSchedules(); publishSchedules(); }
  } else if (strcmp(topic, "smartfarm/restart") == 0) {
    ESP.restart();
  }
}

void connectMQTT() {
  if (mqtt.connected() || millis() - lastMqttReconnect < MQTT_RECONNECT_MS) return;
  lastMqttReconnect = millis();
  char clientId[32]; snprintf(clientId, sizeof(clientId), "SmartFarmV7-%06X", ESP.getChipId());
  if (mqtt.connect(clientId, settings.mqttUser, settings.mqttPass, "smartfarm/status", 1, true, "OFFLINE")) {
    mqttWasConnected = true;
    publishTopic("status", "ONLINE", true);
    publishTopic("firmware", FW_NAME " " FW_VERSION, true);
    mqtt.subscribe("smartfarm/+/set");
    mqtt.subscribe("smartfarm/schedule/set");
    mqtt.subscribe("smartfarm/restart");
    for (uint8_t i = 0; i < RELAY_COUNT; i++) publishRelayState(i);
    publishSchedules();
    notifyTelegram("MQTT Connected");
  } else if (mqttWasConnected) {
    mqttWasConnected = false;
    logEvent("mqtt", "MQTT Lost");
  }
}

void readSensors() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t)) temperature = t;
  if (!isnan(h)) humidity = h;
  soilRaw = analogRead(SOIL_PIN);
  waterLevelOk = digitalRead(WATER_LEVEL_PIN) == HIGH;
  rainDetected = digitalRead(RAIN_PIN) == LOW;
  char payload[160];
  snprintf(payload, sizeof(payload), "{\"temperature\":%.1f,\"humidity\":%.1f,\"soil\":%u,\"waterLevel\":%s,\"rain\":%s,\"heap\":%u}",
           temperature, humidity, soilRaw, waterLevelOk ? "true" : "false", rainDetected ? "true" : "false", ESP.getFreeHeap());
  publishTopic("sensors", payload, true);
  File f = LittleFS.open(FILE_HISTORY, "a");
  if (f) { f.printf("%lu,%.1f,%.1f,%u,%u,%u\n", millis(), temperature, humidity, soilRaw, waterLevelOk, rainDetected); f.close(); }
  if (!waterLevelOk || rainDetected) notifyTelegram("Sensor Alarm");
}

void webStatus() {
  StaticJsonDocument<1024> doc;
  doc["name"] = FW_NAME; doc["version"] = FW_VERSION; doc["wifi"] = WiFi.status() == WL_CONNECTED;
  doc["mqtt"] = mqtt.connected(); doc["rtc"] = rtcOk; doc["ota"] = true; doc["heap"] = ESP.getFreeHeap(); doc["uptime"] = millis() / 1000;
  doc["temperature"] = temperature; doc["humidity"] = humidity; doc["soil"] = soilRaw; doc["waterLevel"] = waterLevelOk; doc["rain"] = rainDetected;
  JsonArray arr = doc.createNestedArray("relays");
  for (uint8_t i = 0; i < RELAY_COUNT; i++) { JsonObject r = arr.createNestedObject(); r["name"] = RELAY_NAMES[i]; r["state"] = relays[i].state; r["mode"] = relays[i].autoRunning ? "Scheduled Auto Running" : "Manual"; r["lastAction"] = relays[i].lastAction; }
  char out[1024]; serializeJson(doc, out, sizeof(out));
  webServer.send(200, "application/json", out);
}

void setupWeb() {
  webServer.on("/api/status", HTTP_GET, webStatus);
  webServer.on("/api/relay", HTTP_POST, []() {
    uint8_t relay = webServer.arg("relay").toInt();
    const bool state = webServer.arg("state") == "ON";
    if (relay < 1 || relay > RELAY_COUNT) { webServer.send(400, "text/plain", "bad relay"); return; }
    setRelay(relay - 1, state, "dashboard");
    webServer.send(200, "text/plain", "ok");
  });
  httpUpdater.setup(&webServer, "/update");
  webServer.begin();
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n===== Smart Farm V7 starting =====");
  ESP.wdtEnable(8000);
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    pinMode(RELAY_PINS[i], OUTPUT);
    digitalWrite(RELAY_PINS[i], RELAY_ACTIVE_LOW ? HIGH : LOW);
    relays[i].state = false;
    relays[i].autoRunning = false;
    relays[i].autoScheduleId = 0;
    relays[i].autoStartedMs = 0;
    relays[i].lastChangeMs = 0;
    safeCopy(relays[i].lastAction, sizeof(relays[i].lastAction), "boot safe off");
  }
  pinMode(WATER_LEVEL_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT_PULLUP);
  LittleFS.begin();
  ensureFile(FILE_SETTINGS, "{}"); ensureFile(FILE_RELAYS, "{\"relays\":[false,false,false,false]}");
  ensureFile(FILE_SCHEDULES, "{\"schedules\":[]}"); ensureFile(FILE_CALIBRATION, "{}");
  loadSettings(); loadRelayState(); loadSchedules();
  for (uint8_t i = 0; i < RELAY_COUNT; i++) digitalWrite(RELAY_PINS[i], RELAY_ACTIVE_LOW ? !relays[i].state : relays[i].state);
  WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true); WiFi.persistent(false);
  WiFiManager wm; wm.setConfigPortalTimeout(180); wm.autoConnect("SmartFarmV7_Setup");
  wifiWasConnected = WiFi.status() == WL_CONNECTED;
  Wire.begin(I2C_SDA, I2C_SCL); rtcOk = rtc.begin(); if (rtcOk && rtc.lostPower()) rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  dht.begin(); tlsClient.setInsecure(); mqtt.setServer(settings.mqttHost, settings.mqttPort); mqtt.setCallback(mqttCallback); mqtt.setBufferSize(1024);
  setupWeb();
  notifyTelegram("ESP Restart");
}

void loop() {
  ESP.wdtFeed();
  webServer.handleClient();
  unsigned long now = millis();
  if (now - lastWifiCheck >= WIFI_CHECK_MS) {
    lastWifiCheck = now;
    bool connected = WiFi.status() == WL_CONNECTED;
    if (connected && !wifiWasConnected) notifyTelegram("WiFi Connected");
    if (!connected && wifiWasConnected) notifyTelegram("WiFi Lost");
    wifiWasConnected = connected;
    if (!connected) WiFi.reconnect();
  }
  if (WiFi.status() == WL_CONNECTED) {
    if (!mqtt.connected()) connectMQTT(); else mqtt.loop();
  }
  if (now - lastRtcTick >= RTC_MS) { lastRtcTick = now; checkSchedules(); }
  if (now - lastSensorRead >= SENSOR_MS) { lastSensorRead = now; readSensors(); }
  if (now - lastHeartbeat >= HEARTBEAT_MS) {
    lastHeartbeat = now;
    char payload[160]; snprintf(payload, sizeof(payload), "{\"uptime\":%lu,\"heap\":%u,\"wifi\":%d,\"rtc\":%d,\"version\":\"%s\"}", millis()/1000, ESP.getFreeHeap(), WiFi.status() == WL_CONNECTED, rtcOk, FW_VERSION);
    publishTopic("heartbeat", payload, false);
  }
  if (now - lastSummary >= SUMMARY_MS) { lastSummary = now; notifyTelegram("Daily summary"); }
  for (uint8_t i = 0; i < RELAY_COUNT; i++) {
    if (relays[i].autoRunning && now - relays[i].autoStartedMs > AUTO_RECOVERY_MS * 180UL) {
      relays[i].autoRunning = false;
      setRelay(i, false, "auto recovery");
    }
  }
  yield();
}
