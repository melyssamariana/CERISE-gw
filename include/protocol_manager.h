#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <map>
#include <vector>
#include <SPIFFS.h>
#include <HTTPClient.h>

// Forward declarations
class CoapPacket;
class UDP;

// Protocol types
enum class ProtocolType {
    MQTT,
    HTTP,
    HTTPS,
    WEBSOCKET,
    COAP,
    CUSTOM
};

// Protocol states
enum class ProtocolState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    ERROR
};

// Message structure
struct ProtocolMessage {
    String topic;
    String payload;
    ProtocolType protocol;
    bool retain;
    uint8_t qos;
    bool isResponse;
};

// Protocol configuration
struct ProtocolConfig {
    // MQTT
    String mqttBroker;
    uint16_t mqttPort;
    String mqttUsername;
    String mqttPassword;
    String mqttClientId;
    String mqttTopicPrefix;
    
    // HTTP/HTTPS
    String httpServer;
    uint16_t httpPort;
    bool useHttps;
    String httpUsername;
    String httpPassword;
    
    // WebSocket
    String wsServer;
    uint16_t wsPort;
    String wsPath;
    bool wsSecure;
    
    // CoAP
    String coapServer;
    uint16_t coapPort;
    
    // Custom
    String customProtocol;
    String customConfig;
};

class ProtocolManager {
public:
    ProtocolManager();
    void begin();
    void update();
    
    // Connection methods
    bool connect(ProtocolType protocol);
    void disconnect(ProtocolType protocol);
    bool isConnected(ProtocolType protocol) const;
    
    // Message handling
    bool publish(const ProtocolMessage& message);
    bool subscribe(const String& topic, ProtocolType protocol);
    void setMessageCallback(void (*callback)(const ProtocolMessage&));
    
    // Configuration
    void setConfig(const ProtocolConfig& config);
    ProtocolConfig getConfig() const;
    bool saveConfig();
    bool loadConfig();
    
    // Protocol-specific methods
    void setMqttCallback(void (*callback)(char*, uint8_t*, unsigned int));
    void setWebSocketCallback(void (*callback)(WStype_t, uint8_t*, size_t));
    void setCoapCallback(void (*callback)(CoapPacket&, IPAddress, int));
    
    // Status methods
    ProtocolState getState(ProtocolType protocol) const;
    String getLastError(ProtocolType protocol) const;

private:
    // Protocol instances
    WiFiClient wifiClient;
    WiFiClientSecure wifiClientSecure;
    PubSubClient mqttClient;
    WebSocketsClient webSocket;
    UDP* udp;
    Coap coap;
    
    // Configuration and state
    ProtocolConfig config;
    std::map<ProtocolType, ProtocolState> states;
    std::map<ProtocolType, String> lastErrors;
    void (*messageCallback)(const ProtocolMessage&);
    
    // Helper methods
    void handleMqttMessage(char* topic, byte* payload, unsigned int length);
    void handleWebSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void handleCoapResponse(CoapPacket& packet, IPAddress ip, int port);
    void setError(ProtocolType protocol, const String& error);
    void clearError(ProtocolType protocol);
    bool validateConfig(const ProtocolConfig& config);
    void logProtocolEvent(ProtocolType protocol, const String& event);
    
    // Connection methods
    bool connectMqtt();
    bool connectHttp(bool useHttps);
    bool connectWebSocket();
    bool connectCoap();
    bool connectCustom();
    
    // Publishing methods
    bool publishHttp(const ProtocolMessage& message);
    bool publishCoap(const ProtocolMessage& message);
    bool publishCustom(const ProtocolMessage& message);
    bool subscribeCoap(const String& topic);
    
    // Message queue
    static const size_t MAX_QUEUE_SIZE = 50;
    std::vector<ProtocolMessage> messageQueue;
    void processMessageQueue();
    bool addToQueue(const ProtocolMessage& message);
};

#endif // PROTOCOL_MANAGER_H 