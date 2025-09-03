#include "protocol_manager.h"
#include <WiFiUdp.h>

ProtocolManager::ProtocolManager()
    : mqttClient(wifiClient)
    , messageCallback(nullptr)
    , udp(new WiFiUDP())
    , coap(*udp)
{
    // Initialize states
    states[ProtocolType::MQTT] = ProtocolState::DISCONNECTED;
    states[ProtocolType::HTTP] = ProtocolState::DISCONNECTED;
    states[ProtocolType::HTTPS] = ProtocolState::DISCONNECTED;
    states[ProtocolType::WEBSOCKET] = ProtocolState::DISCONNECTED;
    states[ProtocolType::COAP] = ProtocolState::DISCONNECTED;
    states[ProtocolType::CUSTOM] = ProtocolState::DISCONNECTED;
    
    // Set default configuration
    config.mqttPort = 1883;
    config.httpPort = 80;
    config.wsPort = 80;
    config.coapPort = 5683;
    config.useHttps = false;
    config.wsSecure = false;
    config.mqttClientId = "CERISE-GW-" + String((uint32_t)ESP.getEfuseMac(), HEX);
}

void ProtocolManager::begin() {
    // Initialize MQTT
    mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
        handleMqttMessage(topic, payload, length);
    });
    
    // Initialize WebSocket
    webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handleWebSocketEvent(type, payload, length);
    });
    
    // Initialize CoAP
    coap.response([this](CoapPacket& packet, IPAddress ip, int port) {
        handleCoapResponse(packet, ip, port);
    });
    coap.start();
    
    // Load configuration
    loadConfig();
}

void ProtocolManager::update() {
    // Update MQTT
    if (states[ProtocolType::MQTT] == ProtocolState::CONNECTED) {
        mqttClient.loop();
    }
    
    // Update WebSocket
    if (states[ProtocolType::WEBSOCKET] == ProtocolState::CONNECTED) {
        webSocket.loop();
    }
    
    // Process message queue
    processMessageQueue();
}

bool ProtocolManager::connect(ProtocolType protocol) {
    switch (protocol) {
        case ProtocolType::MQTT:
            return connectMqtt();
        case ProtocolType::HTTP:
        case ProtocolType::HTTPS:
            return connectHttp(protocol == ProtocolType::HTTPS);
        case ProtocolType::WEBSOCKET:
            return connectWebSocket();
        case ProtocolType::COAP:
            return connectCoap();
        case ProtocolType::CUSTOM:
            return connectCustom();
        default:
            return false;
    }
}

void ProtocolManager::disconnect(ProtocolType protocol) {
    switch (protocol) {
        case ProtocolType::MQTT:
            mqttClient.disconnect();
            break;
        case ProtocolType::WEBSOCKET:
            webSocket.disconnect();
            break;
        case ProtocolType::COAP:
            // CoAP doesn't have a stop method, just mark as disconnected
            break;
        default:
            break;
    }
    states[protocol] = ProtocolState::DISCONNECTED;
}

bool ProtocolManager::isConnected(ProtocolType protocol) const {
    return states.at(protocol) == ProtocolState::CONNECTED;
}

bool ProtocolManager::publish(const ProtocolMessage& message) {
    if (!isConnected(message.protocol)) {
        return addToQueue(message);
    }
    
    switch (message.protocol) {
        case ProtocolType::MQTT:
            return mqttClient.publish(message.topic.c_str(), message.payload.c_str(), message.retain);
            
        case ProtocolType::HTTP:
        case ProtocolType::HTTPS:
            return publishHttp(message);
            
        case ProtocolType::WEBSOCKET: {
            // Create a non-const copy of the payload for WebSocket
            String payloadCopy = message.payload;
            return webSocket.sendTXT(payloadCopy);
        }
            
        case ProtocolType::COAP:
            return publishCoap(message);
            
        case ProtocolType::CUSTOM:
            return publishCustom(message);
            
        default:
            return false;
    }
}

bool ProtocolManager::subscribe(const String& topic, ProtocolType protocol) {
    switch (protocol) {
        case ProtocolType::MQTT:
            return mqttClient.subscribe(topic.c_str());
            
        case ProtocolType::WEBSOCKET:
            // WebSocket doesn't have a subscription mechanism
            return true;
            
        case ProtocolType::COAP:
            return subscribeCoap(topic);
            
        default:
            return false;
    }
}

void ProtocolManager::setMessageCallback(void (*callback)(const ProtocolMessage&)) {
    messageCallback = callback;
}

void ProtocolManager::setConfig(const ProtocolConfig& newConfig) {
    if (validateConfig(newConfig)) {
        config = newConfig;
        saveConfig();
    }
}

ProtocolConfig ProtocolManager::getConfig() const {
    return config;
}

bool ProtocolManager::saveConfig() {
    File file = SPIFFS.open("/protocol_config.json", "w");
    if (!file) {
        return false;
    }

    DynamicJsonDocument doc(2048);
    doc["mqttBroker"] = config.mqttBroker;
    doc["mqttPort"] = config.mqttPort;
    doc["mqttUsername"] = config.mqttUsername;
    doc["mqttPassword"] = config.mqttPassword;
    doc["mqttClientId"] = config.mqttClientId;
    doc["mqttTopicPrefix"] = config.mqttTopicPrefix;
    doc["httpServer"] = config.httpServer;
    doc["httpPort"] = config.httpPort;
    doc["useHttps"] = config.useHttps;
    doc["httpUsername"] = config.httpUsername;
    doc["httpPassword"] = config.httpPassword;
    doc["wsServer"] = config.wsServer;
    doc["wsPort"] = config.wsPort;
    doc["wsPath"] = config.wsPath;
    doc["wsSecure"] = config.wsSecure;
    doc["coapServer"] = config.coapServer;
    doc["coapPort"] = config.coapPort;
    doc["customProtocol"] = config.customProtocol;
    doc["customConfig"] = config.customConfig;

    if (serializeJson(doc, file) == 0) {
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool ProtocolManager::loadConfig() {
    if (!SPIFFS.exists("/protocol_config.json")) {
        return saveConfig();
    }

    File file = SPIFFS.open("/protocol_config.json", "r");
    if (!file) {
        return false;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        return false;
    }

    config.mqttBroker = doc["mqttBroker"] | config.mqttBroker;
    config.mqttPort = doc["mqttPort"] | config.mqttPort;
    config.mqttUsername = doc["mqttUsername"] | config.mqttUsername;
    config.mqttPassword = doc["mqttPassword"] | config.mqttPassword;
    config.mqttClientId = doc["mqttClientId"] | config.mqttClientId;
    config.mqttTopicPrefix = doc["mqttTopicPrefix"] | config.mqttTopicPrefix;
    config.httpServer = doc["httpServer"] | config.httpServer;
    config.httpPort = doc["httpPort"] | config.httpPort;
    config.useHttps = doc["useHttps"] | config.useHttps;
    config.httpUsername = doc["httpUsername"] | config.httpUsername;
    config.httpPassword = doc["httpPassword"] | config.httpPassword;
    config.wsServer = doc["wsServer"] | config.wsServer;
    config.wsPort = doc["wsPort"] | config.wsPort;
    config.wsPath = doc["wsPath"] | config.wsPath;
    config.wsSecure = doc["wsSecure"] | config.wsSecure;
    config.coapServer = doc["coapServer"] | config.coapServer;
    config.coapPort = doc["coapPort"] | config.coapPort;
    config.customProtocol = doc["customProtocol"] | config.customProtocol;
    config.customConfig = doc["customConfig"] | config.customConfig;

    return true;
}

void ProtocolManager::setMqttCallback(void (*callback)(char*, uint8_t*, unsigned int)) {
    mqttClient.setCallback(callback);
}

void ProtocolManager::setWebSocketCallback(void (*callback)(WStype_t, uint8_t*, size_t)) {
    webSocket.onEvent(callback);
}

void ProtocolManager::setCoapCallback(void (*callback)(CoapPacket&, IPAddress, int)) {
    coap.response(callback);
}

ProtocolState ProtocolManager::getState(ProtocolType protocol) const {
    return states.at(protocol);
}

String ProtocolManager::getLastError(ProtocolType protocol) const {
    return lastErrors.at(protocol);
}

// Private methods
bool ProtocolManager::connectMqtt() {
    states[ProtocolType::MQTT] = ProtocolState::CONNECTING;
    
    mqttClient.setServer(config.mqttBroker.c_str(), config.mqttPort);
    
    if (mqttClient.connect(config.mqttClientId.c_str(),
                         config.mqttUsername.c_str(),
                         config.mqttPassword.c_str())) {
        states[ProtocolType::MQTT] = ProtocolState::CONNECTED;
        logProtocolEvent(ProtocolType::MQTT, "Connected to MQTT broker");
        return true;
    } else {
        states[ProtocolType::MQTT] = ProtocolState::ERROR;
        setError(ProtocolType::MQTT, "Failed to connect to MQTT broker");
        return false;
    }
}

bool ProtocolManager::connectHttp(bool useHttps) {
    states[ProtocolType::HTTP] = ProtocolState::CONNECTING;
    
    // HTTP/HTTPS doesn't maintain a persistent connection
    states[ProtocolType::HTTP] = ProtocolState::CONNECTED;
    return true;
}

bool ProtocolManager::connectWebSocket() {
    states[ProtocolType::WEBSOCKET] = ProtocolState::CONNECTING;
    
    if (config.wsSecure) {
        webSocket.beginSSL(config.wsServer.c_str(), config.wsPort, config.wsPath.c_str());
    } else {
        webSocket.begin(config.wsServer.c_str(), config.wsPort, config.wsPath.c_str());
    }
    
    webSocket.onEvent([this](WStype_t type, uint8_t* payload, size_t length) {
        handleWebSocketEvent(type, payload, length);
    });
    
    states[ProtocolType::WEBSOCKET] = ProtocolState::CONNECTED;
    return true;
}

bool ProtocolManager::connectCoap() {
    states[ProtocolType::COAP] = ProtocolState::CONNECTING;
    
    if (coap.start()) {
        states[ProtocolType::COAP] = ProtocolState::CONNECTED;
        return true;
    } else {
        states[ProtocolType::COAP] = ProtocolState::ERROR;
        setError(ProtocolType::COAP, "Failed to start CoAP server");
        return false;
    }
}

bool ProtocolManager::connectCustom() {
    states[ProtocolType::CUSTOM] = ProtocolState::CONNECTING;
    // Implement custom protocol connection
    states[ProtocolType::CUSTOM] = ProtocolState::CONNECTED;
    return true;
}

void ProtocolManager::handleMqttMessage(char* topic, byte* payload, unsigned int length) {
    if (messageCallback) {
        ProtocolMessage message;
        message.topic = String(topic);
        message.payload = String((char*)payload);
        message.protocol = ProtocolType::MQTT;
        message.isResponse = false;
        messageCallback(message);
    }
}

void ProtocolManager::handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            states[ProtocolType::WEBSOCKET] = ProtocolState::DISCONNECTED;
            break;
            
        case WStype_CONNECTED:
            states[ProtocolType::WEBSOCKET] = ProtocolState::CONNECTED;
            break;
            
        case WStype_TEXT:
            if (messageCallback) {
                ProtocolMessage message;
                message.topic = "websocket";
                message.payload = String((char*)payload);
                message.protocol = ProtocolType::WEBSOCKET;
                message.isResponse = false;
                messageCallback(message);
            }
            break;
            
        default:
            break;
    }
}

void ProtocolManager::handleCoapResponse(CoapPacket& packet, IPAddress ip, int port) {
    if (messageCallback) {
        ProtocolMessage message;
        message.topic = String(packet.messageid);
        message.payload = String((char*)packet.payload);
        message.protocol = ProtocolType::COAP;
        message.isResponse = true;
        messageCallback(message);
    }
}

void ProtocolManager::setError(ProtocolType protocol, const String& error) {
    lastErrors[protocol] = error;
    logProtocolEvent(protocol, "Error: " + error);
}

void ProtocolManager::clearError(ProtocolType protocol) {
    lastErrors[protocol] = "";
}

bool ProtocolManager::validateConfig(const ProtocolConfig& config) {
    // Add validation logic here
    return true;
}

void ProtocolManager::logProtocolEvent(ProtocolType protocol, const String& event) {
    String protocolName;
    switch (protocol) {
        case ProtocolType::MQTT:
            protocolName = "MQTT";
            break;
        case ProtocolType::HTTP:
            protocolName = "HTTP";
            break;
        case ProtocolType::HTTPS:
            protocolName = "HTTPS";
            break;
        case ProtocolType::WEBSOCKET:
            protocolName = "WebSocket";
            break;
        case ProtocolType::COAP:
            protocolName = "CoAP";
            break;
        case ProtocolType::CUSTOM:
            protocolName = "Custom";
            break;
    }
    Serial.println("[" + protocolName + "] " + event);
}

void ProtocolManager::processMessageQueue() {
    if (messageQueue.empty()) {
        return;
    }

    auto it = messageQueue.begin();
    while (it != messageQueue.end()) {
        if (publish(*it)) {
            it = messageQueue.erase(it);
        } else {
            ++it;
        }
    }
}

bool ProtocolManager::addToQueue(const ProtocolMessage& message) {
    if (messageQueue.size() >= MAX_QUEUE_SIZE) {
        return false;
    }
    messageQueue.push_back(message);
    return true;
}

bool ProtocolManager::publishHttp(const ProtocolMessage& message) {
    HTTPClient http;
    String url = (config.useHttps ? "https://" : "http://") + config.httpServer + message.topic;
    
    http.begin(url);
    if (!config.httpUsername.isEmpty()) {
        http.setAuthorization(config.httpUsername.c_str(), config.httpPassword.c_str());
    }
    
    int httpCode = http.POST(message.payload);
    http.end();
    
    return httpCode == HTTP_CODE_OK;
}

bool ProtocolManager::publishCoap(const ProtocolMessage& message) {
    return coap.put(IPAddress().fromString(config.coapServer), config.coapPort,
                   message.topic.c_str(), message.payload.c_str());
}

bool ProtocolManager::publishCustom(const ProtocolMessage& message) {
    // Implement custom protocol publishing
    return true;
}

bool ProtocolManager::subscribeCoap(const String& topic) {
    // Implement CoAP subscription
    return true;
} 