// ConnectState.cpp
#include <Arduino.h>
#include "ConnectState.h"
#include "RunState.h"

void ConnectState::enter() {
    Serial.println("[ConnectState] Tentando conectar aos periféricos...");
}

State* ConnectState::update() {
    Serial.println("[ConnectState] Conexão estabelecida. Indo para RunState.");
    return new RunState();
}

void ConnectState::exit() {
    Serial.println("[ConnectState] Saindo do estado de conexão.");
}
