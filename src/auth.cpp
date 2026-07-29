#include "auth.h"
#include "storage.h"
#include "config.h"
#include <Hash.h>
AuthService Auth;
void AuthService::begin(){ randomSeed(ESP.getCycleCount()); snprintf(csrf_,sizeof(csrf_),"%08lx%08lx",random(0xffffffff),random(0xffffffff)); }
String AuthService::sha256(const char *in){ return ::sha256(String(in)); }
bool AuthService::login(const char *u,const char *p,char *out,size_t len){ JsonDocument d; if(!Store.loadJson("/users.json",d))return false; String h=sha256(p); for(JsonObject x:d["users"].as<JsonArray>()){ if(x["enabled"] && strcmp(x["username"]|"",u)==0 && h.equalsIgnoreCase(x["hash"]|"")){ snprintf(token_,sizeof(token_),"%08lx%08lx%08lx%08lx",random(0xffffffff),random(0xffffffff),random(0xffffffff),random(0xffffffff)); role_=x["role"]|2; lastSeen_=millis(); strlcpy(out,token_,len); return true; }} return false; }
bool AuthService::validate(const char *t,Role minRole){ if(!t||strcmp(t,token_)!=0)return false; if(millis()-lastSeen_>SESSION_TIMEOUT_MS)return false; if(role_>minRole)return false; lastSeen_=millis(); return true; }
const char* AuthService::csrf() const{return csrf_;}
void AuthService::logout(const char *t){ if(t&&strcmp(t,token_)==0) token_[0]=0; }
