#include "wifi_manager.h"
#include "../config/config.h"

EspWiFiManager::EspWiFiManager()
    : configured(false), lastReconnectAttempt(0)
{
    generateDeviceId();
}

void EspWiFiManager::generateDeviceId()
{
    uint64_t chipid = ESP.getEfuseMac();
    char buf[32];
    snprintf(buf, sizeof(buf), "microgrow_%04X%08X",
             (uint16_t)(chipid >> 32), (uint32_t)chipid);
    deviceId = String(buf);
}

void EspWiFiManager::setupPortalCallbacks()
{
    wm.setWebServerCallback([this]()
                            { 
        // Device info endpoint (for mobile app)
        wm.server->on("/device-info", HTTP_GET, [this]()
                   {
            String json = "{";
            json += "\"device_id\":\"" + deviceId + "\",";
            json += "\"mac\":\"" + getMacAddress() + "\",";
            json += "\"version\":\"1.0.0\"";
            json += "}";
            wm.server->send(200, "application/json", json);
            Serial.println("Device info requested via HTTP"); });

        // Health check endpoint
        wm.server->on("/ping", HTTP_GET, [this]()
                   { wm.server->send(200, "text/plain", "pong"); }); });
}

bool EspWiFiManager::begin()
{
    WiFi.mode(WIFI_STA);

    // Try to connect with saved credentials
    WiFi.begin();

    Serial.print("Connecting to WiFi");
    uint32_t start = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_TIMEOUT_MS)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        configured = true;
        printStatus();
        return true;
    }

    Serial.println("No saved WiFi credentials");
    return false;
}

bool EspWiFiManager::startConfigPortal()
{
    Serial.println("Starting WiFi configuration portal...");

    WiFi.disconnect(true);
    delay(100);

    wm.setConfigPortalTimeout(WIFI_PORTAL_TIMEOUT_S);
    wm.setConnectTimeout(30);
    wm.setConnectRetries(WIFI_CONNECT_RETRIES);

    // Customize portal
    std::vector<const char *> menu = {"wifi", "info"};
    wm.setMenu(menu);
    wm.setTitle("MicroGrow Setup");
    wm.setHostname(deviceId.c_str());

    // Custom CSS for better mobile experience
    wm.setCustomHeadElement(
        "<style>"
        "body{font-family:sans-serif;text-align:center;background:#1a1a1a;color:#fff;}"
        "button{background:#4CAF50;border:none;color:white;padding:15px 32px;"
        "text-align:center;font-size:16px;margin:4px;border-radius:8px;}"
        "input{padding:10px;margin:5px;border-radius:5px;border:1px solid #555;"
        "background:#333;color:#fff;}"
        "</style>");

    // Setup callbacks for device info endpoint
    setupPortalCallbacks();

    // Display device ID prominently
    String customHTML =
        "<div style='margin:20px;padding:20px;background:#2a2a2a;border-radius:10px;'>"
        "<h3>Device Information</h3>"
        "<p style='font-size:18px;'>Device ID:</p>"
        "<p style='font-size:24px;color:#4CAF50;font-family:monospace;'>" +
        deviceId + "</p>"
                   "<p style='font-size:14px;color:#888;'>Use this ID in the mobile app</p>"
                   "</div>";

    wm.setCustomMenuHTML(customHTML.c_str());
    wm.setBreakAfterConfig(true);

    // Start portal with SSID
    bool success = wm.startConfigPortal("MicroGrow-Setup");

    if (success)
    {
        Serial.println("WiFi configured successfully");
        configured = true;
        printStatus();
        return true;
    }

    Serial.println("WiFi configuration failed or timeout");
    return false;
}

void EspWiFiManager::resetCredentials()
{
    Serial.println("Clearing WiFi credentials...");
    WiFi.disconnect(true);
    wm.resetSettings();
    delay(1000);
    ESP.restart();
}

bool EspWiFiManager::isConnected() const
{
    return WiFi.status() == WL_CONNECTED;
}

bool EspWiFiManager::reconnect()
{
    uint32_t now = millis();

    // Throttle reconnection attempts
    if (now - lastReconnectAttempt < MQTT_RECONNECT_DELAY_MS)
    {
        return false;
    }

    lastReconnectAttempt = now;

    if (isConnected())
    {
        return true;
    }

    Serial.println("Attempting WiFi reconnection...");
    WiFi.disconnect();
    WiFi.begin();

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED &&
           millis() - start < WIFI_TIMEOUT_MS)
    {
        delay(100);
    }

    bool success = isConnected();
    if (success)
    {
        Serial.println("WiFi reconnected");
        printStatus();
    }
    else
    {
        Serial.println("WiFi reconnection failed");
    }

    return success;
}

String EspWiFiManager::getMacAddress() const
{
    return WiFi.macAddress();
}

String EspWiFiManager::getIPAddress() const
{
    return WiFi.localIP().toString();
}

void EspWiFiManager::printStatus() const
{
    Serial.println("=== WiFi Status ===");
    Serial.printf("Device ID: %s\n", deviceId.c_str());
    Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
    Serial.printf("IP: %s\n", getIPAddress().c_str());
    Serial.printf("MAC: %s\n", getMacAddress().c_str());
    Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
    Serial.println("==================");
}
