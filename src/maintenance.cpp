#include "maintenance.h"

Maintenance::Maintenance()
    : currentState(MaintenanceState::IDLE)
    , updateProgress(0.0f)
    , remoteDebugEnabled(false)
    , webServer(80)
    , webServerRunning(false)
{
    // Initialize default configuration
    config.deviceId = "CERISE-GW-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    config.firmwareVersion = "1.0.0";
    config.lastUpdateCheck = "";
    config.autoUpdate = true;
    config.backupPath = "/backup";
    config.updateServer = "https://update.cerise-gw.com";
    config.wifiSSID = "";
    config.wifiPassword = "";
}

void Maintenance::begin() {
    if (!SPIFFS.begin(true)) {
        setError("Failed to mount SPIFFS");
        return;
    }
    
    // Create backup directory if it doesn't exist
    if (!SPIFFS.exists(config.backupPath)) {
        SPIFFS.mkdir(config.backupPath);
    }
    
    // Load configuration
    if (!loadConfig()) {
        setError("Failed to load configuration");
    }

    // Setup web server routes
    webServer.on("/", HTTP_GET, [this]() { handleRoot(); });
    webServer.on("/update", HTTP_POST, [this]() { handleUpdate(); });
    webServer.on("/backup", HTTP_POST, [this]() { handleBackup(); });
    webServer.on("/restore", HTTP_POST, [this]() { handleRestore(); });
    webServer.on("/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
    webServer.on("/config", HTTP_GET, [this]() { handleConfig(); });
    webServer.on("/progress", HTTP_GET, [this]() { handleUpdateProgress(); });
    webServer.on("/status", HTTP_GET, [this]() { handleSystemStatus(); });
    webServer.onNotFound([this]() { handleNotFound(); });
}

void Maintenance::update() {
    if (webServerRunning) {
        webServer.handleClient();
    }

    switch (currentState) {
        case MaintenanceState::CHECKING_UPDATE:
            if (checkForUpdates()) {
                currentState = MaintenanceState::DOWNLOADING_UPDATE;
            } else {
                currentState = MaintenanceState::IDLE;
            }
            break;
            
        case MaintenanceState::DOWNLOADING_UPDATE:
            // Download progress is handled in performUpdate
            break;
            
        case MaintenanceState::INSTALLING_UPDATE:
            // Installation progress is handled in performUpdate
            break;
            
        case MaintenanceState::BACKING_UP:
            if (backupSystem()) {
                currentState = MaintenanceState::IDLE;
            } else {
                currentState = MaintenanceState::ERROR;
            }
            break;
            
        case MaintenanceState::RESTORING:
            if (restoreSystem()) {
                currentState = MaintenanceState::IDLE;
            } else {
                currentState = MaintenanceState::ERROR;
            }
            break;
            
        case MaintenanceState::FACTORY_RESET:
            if (factoryReset()) {
                currentState = MaintenanceState::IDLE;
            } else {
                currentState = MaintenanceState::ERROR;
            }
            break;
            
        default:
            break;
    }
}

void Maintenance::startWebServer() {
    if (!webServerRunning) {
        webServer.begin();
        webServerRunning = true;
        logMaintenanceEvent("Web server started");
    }
}

void Maintenance::stopWebServer() {
    if (webServerRunning) {
        webServer.close();
        webServerRunning = false;
        logMaintenanceEvent("Web server stopped");
    }
}

bool Maintenance::isWebServerRunning() const {
    return webServerRunning;
}

void Maintenance::handleRoot() {
    String html = "<html><body>";
    html += "<h1>CERISE Gateway Maintenance</h1>";
    html += "<p>Device ID: " + config.deviceId + "</p>";
    html += "<p>Firmware Version: " + config.firmwareVersion + "</p>";
    html += "<p><a href='/update'>Check for Updates</a></p>";
    html += "<p><a href='/backup'>Create Backup</a></p>";
    html += "<p><a href='/restore'>Restore from Backup</a></p>";
    html += "<p><a href='/factory-reset'>Factory Reset</a></p>";
    html += "<p><a href='/config'>Configuration</a></p>";
    html += "<p><a href='/status'>System Status</a></p>";
    html += "</body></html>";
    webServer.send(200, "text/html", html);
}

void Maintenance::handleUpdate() {
    if (checkForUpdates()) {
        webServer.send(200, "text/plain", "Update started");
    } else {
        webServer.send(400, "text/plain", "No updates available");
    }
}

void Maintenance::handleBackup() {
    if (backupSystem()) {
        webServer.send(200, "text/plain", "Backup created successfully");
    } else {
        webServer.send(500, "text/plain", "Backup failed: " + lastError);
    }
}

void Maintenance::handleRestore() {
    if (restoreSystem()) {
        webServer.send(200, "text/plain", "System restored successfully");
    } else {
        webServer.send(500, "text/plain", "Restore failed: " + lastError);
    }
}

void Maintenance::handleFactoryReset() {
    if (factoryReset()) {
        webServer.send(200, "text/plain", "Factory reset completed");
    } else {
        webServer.send(500, "text/plain", "Factory reset failed: " + lastError);
    }
}

void Maintenance::handleConfig() {
    DynamicJsonDocument doc(1024);
    doc["deviceId"] = config.deviceId;
    doc["firmwareVersion"] = config.firmwareVersion;
    doc["autoUpdate"] = config.autoUpdate;
    doc["updateServer"] = config.updateServer;
    
    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void Maintenance::handleUpdateProgress() {
    DynamicJsonDocument doc(256);
    doc["progress"] = updateProgress;
    doc["state"] = static_cast<int>(currentState);
    
    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void Maintenance::handleSystemStatus() {
    DynamicJsonDocument doc(512);
    doc["state"] = static_cast<int>(currentState);
    doc["error"] = lastError;
    doc["webServerRunning"] = webServerRunning;
    doc["remoteDebugEnabled"] = remoteDebugEnabled;
    
    String response;
    serializeJson(doc, response);
    webServer.send(200, "application/json", response);
}

void Maintenance::handleNotFound() {
    webServer.send(404, "text/plain", "Not found");
}

bool Maintenance::checkForUpdates() {
    if (WiFi.status() != WL_CONNECTED) {
        setError("No WiFi connection");
        return false;
    }

    HTTPClient http;
    String url = config.updateServer + "/check/" + config.deviceId;
    http.begin(url);
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        setError("Failed to check for updates: " + String(httpCode));
        http.end();
        return false;
    }

    String payload = http.getString();
    http.end();

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        setError("Failed to parse update info");
        return false;
    }

    String latestVersion = doc["version"].as<String>();
    if (latestVersion != config.firmwareVersion) {
        String updateUrl = doc["url"].as<String>();
        return performUpdate(updateUrl);
    }

    return false;
}

bool Maintenance::performUpdate(const String& url) {
    if (WiFi.status() != WL_CONNECTED) {
        setError("No WiFi connection");
        return false;
    }

    HTTPClient http;
    http.begin(url);
    
    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        setError("Failed to download update: " + String(httpCode));
        http.end();
        return false;
    }

    int contentLength = http.getSize();
    if (contentLength <= 0) {
        setError("Invalid update file size");
        http.end();
        return false;
    }

    if (!Update.begin(contentLength)) {
        setError("Not enough space for update");
        http.end();
        return false;
    }

    WiFiClient * stream = http.getStreamPtr();
    size_t written = Update.writeStream(*stream);
    
    if (written == contentLength && Update.end()) {
        logMaintenanceEvent("Update successful");
        ESP.restart();
        return true;
    } else {
        setError("Update failed");
        return false;
    }
}

bool Maintenance::backupSystem() {
    String backupFile = config.backupPath + "/backup_" + String(millis()) + ".json";
    File file = SPIFFS.open(backupFile, "w");
    
    if (!file) {
        setError("Failed to create backup file");
        return false;
    }

    DynamicJsonDocument doc(2048);
    doc["deviceId"] = config.deviceId;
    doc["firmwareVersion"] = config.firmwareVersion;
    doc["timestamp"] = String(millis());
    
    // Add system configuration
    JsonObject systemConfig = doc.createNestedObject("systemConfig");
    systemConfig["autoUpdate"] = config.autoUpdate;
    systemConfig["updateServer"] = config.updateServer;
    
    if (serializeJson(doc, file) == 0) {
        setError("Failed to write backup data");
        file.close();
        return false;
    }

    file.close();
    logMaintenanceEvent("Backup created: " + backupFile);
    return true;
}

bool Maintenance::restoreSystem() {
    File root = SPIFFS.open(config.backupPath);
    if (!root) {
        setError("Failed to open backup directory");
        return false;
    }

    File file = root.openNextFile();
    if (!file) {
        setError("No backup files found");
        return false;
    }

    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, file);
    file.close();
    
    if (error) {
        setError("Failed to parse backup data");
        return false;
    }

    // Restore configuration
    config.deviceId = doc["deviceId"].as<String>();
    config.firmwareVersion = doc["firmwareVersion"].as<String>();
    
    JsonObject systemConfig = doc["systemConfig"];
    config.autoUpdate = systemConfig["autoUpdate"] | true;
    config.updateServer = systemConfig["updateServer"] | config.updateServer;

    if (!saveConfig()) {
        setError("Failed to save restored configuration");
        return false;
    }

    logMaintenanceEvent("System restored from backup");
    return true;
}

bool Maintenance::factoryReset() {
    if (!formatStorage()) {
        setError("Failed to format storage");
        return false;
    }

    // Reset configuration to defaults
    config.deviceId = "CERISE-GW-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    config.firmwareVersion = "1.0.0";
    config.lastUpdateCheck = "";
    config.autoUpdate = true;
    config.backupPath = "/backup";
    config.updateServer = "https://update.cerise-gw.com";

    if (!saveConfig()) {
        setError("Failed to save default configuration");
        return false;
    }

    logMaintenanceEvent("Factory reset completed");
    return true;
}

bool Maintenance::saveConfig() {
    File file = SPIFFS.open("/config.json", "w");
    if (!file) {
        setError("Failed to open config file for writing");
        return false;
    }

    DynamicJsonDocument doc(1024);
    doc["deviceId"] = config.deviceId;
    doc["firmwareVersion"] = config.firmwareVersion;
    doc["lastUpdateCheck"] = config.lastUpdateCheck;
    doc["autoUpdate"] = config.autoUpdate;
    doc["backupPath"] = config.backupPath;
    doc["updateServer"] = config.updateServer;

    if (serializeJson(doc, file) == 0) {
        setError("Failed to write config file");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool Maintenance::loadConfig() {
    if (!SPIFFS.exists("/config.json")) {
        return saveConfig();
    }

    File file = SPIFFS.open("/config.json", "r");
    if (!file) {
        setError("Failed to open config file");
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    file.close();

    if (error) {
        setError("Failed to parse config file");
        return false;
    }

    config.deviceId = doc["deviceId"] | config.deviceId;
    config.firmwareVersion = doc["firmwareVersion"] | config.firmwareVersion;
    config.lastUpdateCheck = doc["lastUpdateCheck"] | config.lastUpdateCheck;
    config.autoUpdate = doc["autoUpdate"] | config.autoUpdate;
    config.backupPath = doc["backupPath"] | config.backupPath;
    config.updateServer = doc["updateServer"] | config.updateServer;

    return true;
}

void Maintenance::setConfig(const SystemConfig& newConfig) {
    config = newConfig;
    saveConfig();
}

SystemConfig Maintenance::getConfig() const {
    return config;
}

void Maintenance::enableRemoteDebug(bool enable) {
    remoteDebugEnabled = enable;
}

bool Maintenance::isRemoteDebugEnabled() const {
    return remoteDebugEnabled;
}

MaintenanceState Maintenance::getState() const {
    return currentState;
}

String Maintenance::getLastError() const {
    return lastError;
}

float Maintenance::getUpdateProgress() const {
    return updateProgress;
}

bool Maintenance::formatStorage() {
    return SPIFFS.format();
}

void Maintenance::setError(const String& error) {
    lastError = error;
    logMaintenanceEvent("Error: " + error);
}

void Maintenance::clearError() {
    lastError = "";
}

void Maintenance::logMaintenanceEvent(const String& event) {
    // TODO: Implement proper logging system
    Serial.println("[Maintenance] " + event);
} 