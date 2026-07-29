#include "wifi.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <time.h>
WifiService FarmWifi;
static WiFiManager wm;
void WifiService::begin(){ WiFi.mode(WIFI_STA); WiFi.setAutoReconnect(true); WiFi.hostname(DEVICE_HOSTNAME); wm.setConfigPortalBlocking(false); wm.autoConnect(AP_SSID); configTime(TZ_OFFSET_SEC, DST_OFFSET_SEC, NTP_SERVER_1, NTP_SERVER_2); MDNS.begin(DEVICE_HOSTNAME); }
void WifiService::loop(){ wm.process(); MDNS.update(); }
bool WifiService::connected() const{return WiFi.status()==WL_CONNECTED;}
int WifiService::rssi() const{return WiFi.RSSI();}
String WifiService::ip() const{return WiFi.localIP().toString();}
