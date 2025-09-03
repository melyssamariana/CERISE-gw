#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include <Arduino.h>
#include <Update.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <WebServer.h>

// Maintenance states
enum class MaintenanceState {
    IDLE,
    CHECKING_UPDATE,
    DOWNLOADING_UPDATE,
    INSTALLING_UPDATE,
    BACKING_UP,
    RESTORING,
    FACTORY_RESET,
    ERROR
};

// Configuration structure
struct SystemConfig {
    String deviceId;
    String firmwareVersion;
    String lastUpdateCheck;
    bool autoUpdate;
    String backupPath;
    String updateServer;
    String wifiSSID;
    String wifiPassword;
};

class Maintenance {
public:
    Maintenance();
    void begin();
    void update();
    
    // OTA Update methods
    bool checkForUpdates();
    bool performUpdate(const String& url);
    void setUpdateServer(const String& server);
    
    // Backup and Restore methods
    bool backupSystem();
    bool restoreSystem();
    bool factoryReset();
    
    // Configuration methods
    bool saveConfig();
    bool loadConfig();
    void setConfig(const SystemConfig& config);
    SystemConfig getConfig() const;
    
    // Debug methods
    void enableRemoteDebug(bool enable);
    bool isRemoteDebugEnabled() const;
    
    // Status methods
    MaintenanceState getState() const;
    String getLastError() const;
    float getUpdateProgress() const;

    // Web interface methods
    void startWebServer();
    void stopWebServer();
    bool isWebServerRunning() const;

private:
    MaintenanceState currentState;
    SystemConfig config;
    String lastError;
    float updateProgress;
    bool remoteDebugEnabled;
    WebServer webServer;
    bool webServerRunning;
    
    // Helper methods
    bool downloadFile(const String& url, const String& path);
    bool verifyUpdate(const String& path);
    bool installUpdate(const String& path);
    void clearError();
    void setError(const String& error);
    bool formatStorage();
    bool backupConfig();
    bool restoreConfig();
    void logMaintenanceEvent(const String& event);
    
    // Web server handlers
    void handleRoot();
    void handleUpdate();
    void handleBackup();
    void handleRestore();
    void handleFactoryReset();
    void handleConfig();
    void handleNotFound();
    void handleUpdateProgress();
    void handleSystemStatus();
};

#endif // MAINTENANCE_H 