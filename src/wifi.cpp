#include "wifi.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
WifiService FarmWifi;
void WifiService::begin(){ WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true); WiFi.hostname(DEVICE_HOSTNAME); WiFiManager wm; wm.setConfigPortalBlocking(false); wm.autoConnect(AP_SSID); MDNS.begin(DEVICE_HOSTNAME); }
void WifiService::loop(){ MDNS.update(); }
bool WifiService::connected() const{return WiFi.status()==WL_CONNECTED;}
int WifiService::rssi() const{return WiFi.RSSI();}
String WifiService::ip() const{return WiFi.localIP().toString();}
