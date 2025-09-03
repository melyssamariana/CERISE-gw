// InitState.cpp
#include <Arduino.h>
#include "InitState.h"
#include "ConnectState.h"

void InitState::enter() {
    Serial.println("[InitState] Inicializando hardware...");
}

State* InitState::update() {
    Serial.println("[InitState] Completado. Indo para ConnectState.");
    return new ConnectState();
}

void InitState::exit() {
    Serial.println("[InitState] Saindo do estado de inicialização.");
}
