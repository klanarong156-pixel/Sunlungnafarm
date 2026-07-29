#include "websocket.h"
#include "config.h"
#include "wifi.h"
#include "relay.h"
#include "scheduler.h"
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
static WebSocketsServer server(WS_PORT); static uint32_t lastPush=0; FarmWebSocket WS;
void FarmWebSocket::begin(){ server.begin(); }
void FarmWebSocket::loop(){ server.loop(); if(millis()-lastPush>WS_PUSH_MS){ lastPush=millis(); broadcastState(); }}
void FarmWebSocket::broadcastState(){ JsonDocument d; d["wifi"]=FarmWifi.connected(); d["rssi"]=FarmWifi.rssi(); d["ip"]=FarmWifi.ip(); d["uptime"]=millis()/1000; d["heap"]=ESP.getFreeHeap(); d["mode"]=FarmScheduler.autoMode()?"Auto":"Manual"; JsonArray r=d["relays"].to<JsonArray>(); for(uint8_t i=0;i<RELAY_COUNT;i++)r.add(Relays.state(i)); char out[384]; size_t n=serializeJson(d,out); server.broadcastTXT(out,n); }
