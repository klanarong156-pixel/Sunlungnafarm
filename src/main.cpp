#include <Arduino.h>
#include "logger.h"
#include "storage.h"
#include "auth.h"
#include "relay.h"
#include "wifi.h"
#include "scheduler.h"
#include "webserver.h"
#include "websocket.h"
#include "ota.h"
#include <ArduinoOTA.h>
void setup(){ Log.begin(); Store.begin(); Store.ensureDefaults(); Auth.begin(); Relays.begin(); FarmWifi.begin(); FarmScheduler.begin(); Web.begin(); WS.begin(); OTAService.begin(); Log.event(F("Smart Farm booted")); }
void loop(){ FarmWifi.loop(); FarmScheduler.loop(); Web.loop(); WS.loop(); ArduinoOTA.handle(); yield(); }
