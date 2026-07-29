#pragma once
#include <Arduino.h>
enum Role:uint8_t{ROLE_SUPER_ADMIN=0,ROLE_ADMIN=1,ROLE_USER=2};
class AuthService{ public: void begin(); bool login(const char *user,const char *pass,char *token,size_t len); bool validate(const char *token,Role minRole); const char* csrf() const; void logout(const char *token); String sha256(const char *input); private: char token_[33]={0}; char csrf_[17]={0}; uint32_t lastSeen_=0; uint8_t role_=ROLE_USER;};
extern AuthService Auth;
