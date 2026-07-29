#include "ota.h"
#include <ArduinoOTA.h>
#include "config.h"
OtaService OTAService;
void OtaService::begin(){ ArduinoOTA.setHostname(DEVICE_HOSTNAME); ArduinoOTA.begin(); }
