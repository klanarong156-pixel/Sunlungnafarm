#pragma once
#include <Arduino.h>
struct ScheduleItem{ bool enabled; uint8_t relay; uint8_t daysMask; uint16_t startMinute; uint16_t durationMinute; bool running; };
class Scheduler{ public: void begin(); void loop(); void setAutoMode(bool on); bool autoMode() const; bool save(); bool load(); private: bool autoMode_=false; uint32_t lastTick_=0; ScheduleItem items_[8]; uint8_t count_=0; uint16_t minuteNow(); uint8_t dayMaskNow();};
extern Scheduler FarmScheduler;
