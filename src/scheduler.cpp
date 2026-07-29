#include "scheduler.h"
#include "storage.h"
#include "relay.h"
#include "config.h"
#include <time.h>
Scheduler FarmScheduler;
static bool clockReady(){ time_t t=time(nullptr); return t > 1704067200; }
void Scheduler::begin(){ load(); }
bool Scheduler::load(){ JsonDocument d; if(!Store.loadJson("/schedule.json",d)){ count_=0; return true;} autoMode_=d["autoMode"]|false; count_=0; for(JsonObject o:d["items"].as<JsonArray>()){ if(count_>=8)break; items_[count_++]={o["enabled"]|false,o["relay"]|1,o["days"]|127,o["start"]|360,o["duration"]|10,false}; } return true; }
bool Scheduler::save(){ JsonDocument d; d["autoMode"]=autoMode_; JsonArray a=d["items"].to<JsonArray>(); for(uint8_t i=0;i<count_;i++){ JsonObject o=a.add<JsonObject>(); o["enabled"]=items_[i].enabled; o["relay"]=items_[i].relay; o["days"]=items_[i].daysMask; o["start"]=items_[i].startMinute; o["duration"]=items_[i].durationMinute;} return Store.saveJson("/schedule.json",d); }
void Scheduler::setAutoMode(bool on){ autoMode_=on; save(); }
bool Scheduler::autoMode() const{return autoMode_;}
uint16_t Scheduler::minuteNow(){ time_t t=time(nullptr); struct tm *n=localtime(&t); return n? n->tm_hour*60+n->tm_min:0; }
uint8_t Scheduler::dayMaskNow(){ time_t t=time(nullptr); struct tm *n=localtime(&t); return n? (1<<n->tm_wday):0; }
void Scheduler::loop(){ if(!autoMode_ || millis()-lastTick_<SCHEDULER_TICK_MS)return; lastTick_=millis(); if(!clockReady()){ Relays.enforceSafety(); return; } uint16_t m=minuteNow(); uint8_t dm=dayMaskNow(); for(uint8_t i=0;i<count_;i++){ ScheduleItem &s=items_[i]; bool active=s.enabled && (s.daysMask&dm) && m>=s.startMinute && m<(uint16_t)(s.startMinute+s.durationMinute); if(active!=s.running){ s.running=active; Relays.set(s.relay,active,false); }} Relays.enforceSafety(); }
