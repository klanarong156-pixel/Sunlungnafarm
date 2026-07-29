#include "relay.h"
#include "config.h"
RelayController Relays;
void RelayController::begin(){ for(uint8_t i=0;i<RELAY_COUNT;i++){ pinMode(RELAY_PINS[i],OUTPUT); states_[i]=false; writePin(i);} }
void RelayController::writePin(uint8_t i){ digitalWrite(RELAY_PINS[i], (states_[i] ^ !RELAY_ACTIVE_LOW) ? LOW : HIGH); }
bool RelayController::set(uint8_t i,bool on,bool){ if(i>=RELAY_COUNT)return false; if(i==0 && on && !anyZoneOn()) return false; states_[i]=on; writePin(i); enforceSafety(); return true; }
void RelayController::emergencyStop(){ for(uint8_t i=0;i<RELAY_COUNT;i++){ states_[i]=false; writePin(i);} }
bool RelayController::state(uint8_t i) const { return i<RELAY_COUNT && states_[i]; }
bool RelayController::anyZoneOn() const { return states_[1]; }
void RelayController::enforceSafety(){ if(!anyZoneOn() && states_[0]){ states_[0]=false; writePin(0);} }
bool RelayController::pumpOn() const { return states_[0]; }
