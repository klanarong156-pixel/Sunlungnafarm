#pragma once
#include <Arduino.h>
class FarmWebSocket{ public: void begin(); void loop(); void broadcastState();};
extern FarmWebSocket WS;
