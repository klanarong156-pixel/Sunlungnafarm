#pragma once
#include <Arduino.h>
class RelayController {
 public:
  void begin();
  bool set(uint8_t index, bool on, bool manualOverride);
  void emergencyStop();
  bool state(uint8_t index) const;
  bool anyZoneOn() const;
  void enforceSafety();
  bool pumpOn() const;
 private:
  bool states_[4] = {false, false, false, false};
  void writePin(uint8_t index);
};
extern RelayController Relays;
