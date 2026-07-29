#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
struct UserRecord{ char username[24]; char passHash[65]; uint8_t role; bool enabled; };
class Storage { public: bool begin(); bool loadJson(const char *path, JsonDocument &doc); bool saveJson(const char *path, JsonDocument &doc); void ensureDefaults();};
extern Storage Store;
