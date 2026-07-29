#pragma once
#include <Arduino.h>
class Logger { public: void begin(); void event(const __FlashStringHelper *msg); void error(const __FlashStringHelper *msg); String readLog(const char *path); private: void append(const char *path,const __FlashStringHelper *msg);};
extern Logger Log;
