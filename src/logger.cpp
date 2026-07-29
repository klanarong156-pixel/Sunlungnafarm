#include "logger.h"
#include "config.h"
#include <LittleFS.h>
Logger Log;
void Logger::begin(){ if(DEBUG_SERIAL) Serial.begin(115200); LittleFS.begin(); }
void Logger::append(const char *p,const __FlashStringHelper *m){ File f=LittleFS.open(p,"a"); if(f){ f.printf("%lu ",millis()); f.println(m); f.close(); } if(DEBUG_SERIAL) Serial.println(m); }
void Logger::event(const __FlashStringHelper *m){ append("/event.log",m); }
void Logger::error(const __FlashStringHelper *m){ append("/error.log",m); }
String Logger::readLog(const char *p){ File f=LittleFS.open(p,"r"); if(!f)return String(); String s=f.readString(); f.close(); return s; }
