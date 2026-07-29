#pragma once
#include <Arduino.h>
class WifiService{ public: void begin(); void loop(); bool connected() const; int rssi() const; String ip() const;};
extern WifiService FarmWifi;
