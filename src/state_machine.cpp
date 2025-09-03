#include "state_machine.h"

StateMachine::StateMachine()
    : currentState(SystemState::INIT)
    , loraState(LoraState::IDLE)
    , zigbeeState(ZigbeeState::IDLE)
    , modbusState(ModbusState::IDLE)
    , analogState(AnalogState::IDLE)
    , loraSerial(LORA_RX_PIN, LORA_TX_PIN)
    , zigbeeSerial(ZIGBEE_RX_PIN, ZIGBEE_TX_PIN)
    , led(LED_COUNT, LED_RGB_PIN, NEO_GRB + NEO_KHZ800)
{
}

void StateMachine::begin() {
    // Initialize all modules
    initLora();
    initZigbee();
    initModbus();
    initAnalog();
    initLed();
    initMaintenance();
    
    // Set initial state
    setState(SystemState::INIT);
}

void StateMachine::update() {
    switch (currentState) {
        case SystemState::INIT:
            // Initialize all modules and move to configuration
            setState(SystemState::LORA_CONFIG);
            break;

        case SystemState::LORA_CONFIG:
            updateLora();
            if (loraState == LoraState::IDLE) {
                setState(SystemState::ZIGBEE_CONFIG);
            }
            break;

        case SystemState::ZIGBEE_CONFIG:
            updateZigbee();
            if (zigbeeState == ZigbeeState::IDLE) {
                setState(SystemState::MODBUS_CONFIG);
            }
            break;

        case SystemState::MODBUS_CONFIG:
            updateModbus();
            if (modbusState == ModbusState::IDLE) {
                setState(SystemState::ANALOG_READING);
            }
            break;

        case SystemState::ANALOG_READING:
            updateAnalog();
            if (analogState == AnalogState::IDLE) {
                setState(SystemState::DATA_PROCESSING);
            }
            break;

        case SystemState::DATA_PROCESSING:
            // Process collected data
            updateLora();
            updateZigbee();
            updateModbus();
            updateAnalog();
            updateLed();
            updateMaintenance();
            break;

        case SystemState::MAINTENANCE:
            updateMaintenance();
            if (maintenance.getState() == MaintenanceState::IDLE) {
                setState(SystemState::DATA_PROCESSING);
            }
            break;

        case SystemState::ERROR:
            handleError();
            break;
    }
}

void StateMachine::setState(SystemState newState) {
    currentState = newState;
    // Update LED color based on state
    switch (newState) {
        case SystemState::INIT:
            updateLedColor(0, 0, 255); // Blue
            break;
        case SystemState::MAINTENANCE:
            updateLedColor(255, 165, 0); // Orange
            break;
        case SystemState::ERROR:
            updateLedColor(255, 0, 0); // Red
            break;
        default:
            updateLedColor(0, 255, 0); // Green
            break;
    }
}

SystemState StateMachine::getCurrentState() const {
    return currentState;
}

void StateMachine::initLora() {
    pinMode(LORA_AUX_PIN, INPUT);
    pinMode(LORA_M0_PIN, OUTPUT);
    pinMode(LORA_M1_PIN, OUTPUT);
    
    // Set to configuration mode
    digitalWrite(LORA_M0_PIN, HIGH);
    digitalWrite(LORA_M1_PIN, HIGH);
    
    loraSerial.begin(9600);
    loraState = LoraState::CONFIGURING;
}

void StateMachine::initZigbee() {
    zigbeeSerial.begin(9600);
    zigbeeState = ZigbeeState::CONFIGURING;
}

void StateMachine::initModbus() {
    pinMode(MODBUS_DE_PIN, OUTPUT);
    pinMode(MODBUS_RE_PIN, OUTPUT);
    
    // Set to receive mode
    digitalWrite(MODBUS_DE_PIN, LOW);
    digitalWrite(MODBUS_RE_PIN, LOW);
    
    //modbus.begin(9600); //TODO: Set the correct baud rate
    modbusState = ModbusState::IDLE;
}

void StateMachine::initAnalog() {
    pinMode(ANALOG_INPUT_1, INPUT);
    pinMode(ANALOG_INPUT_2, INPUT);
    analogState = AnalogState::IDLE;
}

void StateMachine::initLed() {
    led.begin();
    led.setBrightness(50);
    updateLedColor(0, 0, 255); // Blue for initialization
}

void StateMachine::initMaintenance() {
    maintenance.begin();
}

void StateMachine::updateLora() {
    switch (loraState) {
        case LoraState::CONFIGURING:
            // Configure LORA module
            // Add your configuration commands here
            loraState = LoraState::IDLE;
            break;
            
        case LoraState::TRANSMITTING:
            // Handle transmission
            break;
            
        case LoraState::RECEIVING:
            // Handle reception
            break;
            
        case LoraState::ERROR:
            handleError();
            break;
            
        default:
            break;
    }
}

void StateMachine::updateZigbee() {
    switch (zigbeeState) {
        case ZigbeeState::CONFIGURING:
            // Configure Zigbee module
            // Add your configuration commands here
            zigbeeState = ZigbeeState::IDLE;
            break;
            
        case ZigbeeState::TRANSMITTING:
            // Handle transmission
            break;
            
        case ZigbeeState::RECEIVING:
            // Handle reception
            break;
            
        case ZigbeeState::ERROR:
            handleError();
            break;
            
        default:
            break;
    }
}

void StateMachine::updateModbus() {
    switch (modbusState) {
        case ModbusState::READING:
            // Set to receive mode
            digitalWrite(MODBUS_DE_PIN, LOW);
            digitalWrite(MODBUS_RE_PIN, LOW);
            // Add your Modbus reading logic here
            break;
            
        case ModbusState::WRITING:
            // Set to transmit mode
            digitalWrite(MODBUS_DE_PIN, HIGH);
            digitalWrite(MODBUS_RE_PIN, HIGH);
            // Add your Modbus writing logic here
            break;
            
        case ModbusState::ERROR:
            handleError();
            break;
            
        default:
            break;
    }
}

void StateMachine::updateAnalog() {
    switch (analogState) {
        case AnalogState::READING:
        {
            // Read analog inputs
            int value1 = analogRead(ANALOG_INPUT_1);
            int value2 = analogRead(ANALOG_INPUT_2);
            // Process the values as needed
            analogState = AnalogState::IDLE;
            break;
        }
            
        case AnalogState::ERROR:
            handleError();
            break;
            
        default:
            break;
    }
}

void StateMachine::updateLed() {
    led.show();
}

void StateMachine::updateMaintenance() {
    maintenance.update();
}

void StateMachine::handleError() {
    updateLedColor(255, 0, 0); // Red for error
    // Add error handling logic here
}

void StateMachine::updateLedColor(uint8_t r, uint8_t g, uint8_t b) {
    led.setPixelColor(0, r, g, b);
    led.show();
}

// Maintenance methods
void StateMachine::checkForUpdates() {
    setState(SystemState::MAINTENANCE);
    maintenance.checkForUpdates();
}

void StateMachine::backupSystem() {
    setState(SystemState::MAINTENANCE);
    maintenance.backupSystem();
}

void StateMachine::restoreSystem() {
    setState(SystemState::MAINTENANCE);
    maintenance.restoreSystem();
}

void StateMachine::factoryReset() {
    setState(SystemState::MAINTENANCE);
    maintenance.factoryReset();
}

void StateMachine::enableRemoteDebug(bool enable) {
    maintenance.enableRemoteDebug(enable);
}

MaintenanceState StateMachine::getMaintenanceState() const {
    return maintenance.getState();
}

String StateMachine::getMaintenanceError() const {
    return maintenance.getLastError();
}

float StateMachine::getUpdateProgress() const {
    return maintenance.getUpdateProgress();
} 