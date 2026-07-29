#include "storage.h"
#include <LittleFS.h>
Storage Store;
bool Storage::begin(){ return LittleFS.begin(); }
bool Storage::loadJson(const char *p, JsonDocument &d){ File f=LittleFS.open(p,"r"); if(!f)return false; DeserializationError e=deserializeJson(d,f); f.close(); return !e; }
bool Storage::saveJson(const char *p, JsonDocument &d){ File f=LittleFS.open(p,"w"); if(!f)return false; serializeJson(d,f); f.close(); return true; }
void Storage::ensureDefaults(){ if(!LittleFS.exists("/users.json")){ JsonDocument d; JsonArray a=d["users"].to<JsonArray>(); JsonObject u=a.add<JsonObject>(); u["username"]="admin"; u["hash"]="240be518fabd2724d134c13d07dca6a3c81946bcd638b7e3335a1762d2d88f9b"; u["role"]=0; u["enabled"]=true; saveJson("/users.json",d);} if(!LittleFS.exists("/settings.json")){ JsonDocument d; d["autoMode"]=false; saveJson("/settings.json",d);} }
