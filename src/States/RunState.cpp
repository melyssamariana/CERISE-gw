// RunState.cpp
#include <Arduino.h>
#include "RunState.h"

void RunState::enter() {
    Serial.println("[RunState] Sistema operacional iniciado.");
}

State* RunState::update() {
    Serial.println("[RunState] Executando tarefas principais...");
    delay(1000); 
    return this;
}

void RunState::exit() {
    Serial.println("[RunState] Encerrando execução.");
}
