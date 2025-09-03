#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include <SoftwareSerial.h>
#include <ModbusMaster.h>
#include <Adafruit_NeoPixel.h>
#include "maintenance.h"

// Pin Definitions
// LORA Module (E220-900T22D)
#define LORA_TX_PIN 17
#define LORA_RX_PIN 16
#define LORA_AUX_PIN 4
#define LORA_M0_PIN 2
#define LORA_M1_PIN 15

// ZIGBEE Module (XBee-S2C)
#define ZIGBEE_TX_PIN 18
#define ZIGBEE_RX_PIN 19

// MODBUS Module (MAX485)
#define MODBUS_TX_PIN 21
#define MODBUS_RX_PIN 22
#define MODBUS_DE_PIN 23
#define MODBUS_RE_PIN 5

// 4-20mA Inputs (HW-685)
#define ANALOG_INPUT_1 34
#define ANALOG_INPUT_2 35

// LED RGB
#define LED_RGB_PIN 13
#define LED_COUNT 1

// States
enum class SystemState {
    INIT,
    LORA_CONFIG,
    ZIGBEE_CONFIG,
    MODBUS_CONFIG,
    ANALOG_READING,
    DATA_PROCESSING,
    MAINTENANCE,
    ERROR
};

// Module States
enum class LoraState {
    IDLE,
    CONFIGURING,
    TRANSMITTING,
    RECEIVING,
    ERROR
};

enum class ZigbeeState {
    IDLE,
    CONFIGURING,
    TRANSMITTING,
    RECEIVING,
    ERROR
};

enum class ModbusState {
    IDLE,
    READING,
    WRITING,
    ERROR
};

enum class AnalogState {
    IDLE,
    READING,
    ERROR
};

class StateMachine {
public:
    StateMachine();
    void begin();
    void update();
    void setState(SystemState newState);
    SystemState getCurrentState() const;
    
    // Maintenance methods
    void checkForUpdates();
    void backupSystem();
    void restoreSystem();
    void factoryReset();
    void enableRemoteDebug(bool enable);
    MaintenanceState getMaintenanceState() const;
    String getMaintenanceError() const;
    float getUpdateProgress() const;

private:
    // State variables
    SystemState currentState;
    LoraState loraState;
    ZigbeeState zigbeeState;
    ModbusState modbusState;
    AnalogState analogState;

    // Module instances
    SoftwareSerial loraSerial;
    SoftwareSerial zigbeeSerial;
    ModbusMaster modbus;
    Adafruit_NeoPixel led;
    Maintenance maintenance;

    // Module methods
    void initLora();
    void initZigbee();
    void initModbus();
    void initAnalog();
    void initLed();
    void initMaintenance();

    void updateLora();
    void updateZigbee();
    void updateModbus();
    void updateAnalog();
    void updateLed();
    void updateMaintenance();

    // Helper methods
    void handleError();
    void updateLedColor(uint8_t r, uint8_t g, uint8_t b);
};

#endif // STATE_MACHINE_H 