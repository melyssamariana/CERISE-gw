// main.cpp
#include <Arduino.h>
#include "state_machine.h"

StateMachine stateMachine;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting CERISE Gateway...");
    
    stateMachine.begin();
}

void loop() {
    stateMachine.update();
    delay(10); // Small delay to prevent CPU hogging
}
