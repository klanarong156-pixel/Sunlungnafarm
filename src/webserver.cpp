#include "webserver.h"
#include "config.h"
#include "auth.h"
#include "relay.h"
#include "scheduler.h"
#include "logger.h"
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <Updater.h>
static ESP8266WebServer srv(HTTP_PORT); FarmWebServer Web;
static bool otaAuthorized = false;
static bool require(Role r){ return Auth.validate(srv.header("X-Session").c_str(),r) && srv.header("X-CSRF")==Auth.csrf(); }
static void sendFile(const char *p,const char *type){ File f=LittleFS.open(p,"r"); if(!f){ srv.send(404,"text/plain",""); return;} srv.streamFile(f,type); f.close(); }
void FarmWebServer::begin(){ srv.collectHeaders((const char*[]){"X-Session","X-CSRF"},2);
 srv.on("/",[](){sendFile("/login.html","text/html");}); srv.on("/dashboard.html",[](){sendFile("/dashboard.html","text/html");}); srv.on("/style.css",[](){sendFile("/style.css","text/css");}); srv.on("/script.js",[](){sendFile("/script.js","application/javascript");});
 srv.on("/api/login",HTTP_POST,[](){ char t[33]; if(Auth.login(srv.arg("user").c_str(),srv.arg("pass").c_str(),t,sizeof(t))) srv.send(200,"application/json",String("{\"token\":\"")+t+"\",\"csrf\":\""+Auth.csrf()+"\"}"); else srv.send(401,"application/json","{}"); });
 srv.on("/api/logout",HTTP_POST,[](){ Auth.logout(srv.header("X-Session").c_str()); srv.send(204); });
 srv.on("/api/relay",HTTP_POST,[](){ if(!require(ROLE_USER)){srv.send(403);return;} uint8_t id=srv.arg("id").toInt(); bool on=srv.arg("on")=="1"; srv.send(Relays.set(id,on,true)?200:409,"application/json","{}"); });
 srv.on("/api/estop",HTTP_POST,[](){ if(!require(ROLE_ADMIN)){srv.send(403);return;} Relays.emergencyStop(); Log.event(F("Emergency stop")); srv.send(200,"application/json","{}"); });
 srv.on("/api/mode",HTTP_POST,[](){ if(!require(ROLE_ADMIN)){srv.send(403);return;} FarmScheduler.setAutoMode(srv.arg("auto")=="1"); srv.send(200,"application/json","{}"); });
 srv.on("/logs/event",[](){ if(!require(ROLE_ADMIN)){srv.send(403);return;} srv.send(200,"text/plain",Log.readLog("/event.log")); });
 srv.on("/update",HTTP_POST,[](){ if(!require(ROLE_ADMIN) || !otaAuthorized){ otaAuthorized=false; srv.send(403); return; } if(Update.hasError()) srv.send(500,"text/plain","OTA failed"); else { srv.send(200,"text/plain","OK"); otaAuthorized=false; ESP.restart(); } otaAuthorized=false; }, [](){ HTTPUpload &u=srv.upload(); if(u.status==UPLOAD_FILE_START){ otaAuthorized=require(ROLE_ADMIN); if(otaAuthorized) Update.begin((ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000); } else if(!otaAuthorized) return; else if(u.status==UPLOAD_FILE_WRITE) Update.write(u.buf,u.currentSize); else if(u.status==UPLOAD_FILE_END) Update.end(true); });
 srv.begin(); }
void FarmWebServer::loop(){ srv.handleClient(); }
