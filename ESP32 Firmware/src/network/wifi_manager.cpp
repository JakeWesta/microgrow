#include "wifi_manager.h"
#include <config/config.h>

EspWiFiManager::EspWiFiManager()
    : configured(false), lastReconnectAttempt(0)
{
    generateDeviceId();
}

void EspWiFiManager::generateDeviceId()
{
    char buf[17];
    sprintf(buf, "%012llX", ESP.getEfuseMac());
    deviceId = buf;
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

bool EspWiFiManager::runConfigPortal(std::function<bool()> shouldAbort)
{
    Serial.println("Starting WiFi configuration portal...");

    WiFi.disconnect(true);
    delay(100);

    wm.setConnectTimeout(30);
    wm.setConnectRetries(WIFI_CONNECT_RETRIES);

    // Customize portal appearance
    std::vector<const char *> menu = {"wifi", "info"};
    wm.setMenu(menu);
    wm.setTitle("MicroGrow Setup");
    wm.setHostname(deviceId.c_str());

    // Non-blocking mode - we drive the loop ourselves so we can check
    // the abort predicate on every iteration
    wm.setConfigPortalBlocking(false);
    wm.startConfigPortal("MicroGrow Setup");

    while (wm.getConfigPortalActive())
    {
        if (shouldAbort && shouldAbort())
        {
            Serial.println("Config portal aborted by caller");
            wm.stopConfigPortal();
            return false;
        }
        wm.process();
        vTaskDelay(pdMS_TO_TICKS(10)); // yield to FreeRTOS scheduler
    }

    bool success = (WiFi.status() == WL_CONNECTED);
    if (success)
    {
        Serial.println("WiFi configured successfully");
        configured = true;
        printStatus();
    }
    else
    {
        Serial.println("WiFi configuration failed or timeout");
    }

    return success;
}

void EspWiFiManager::resetCredentials()
{
    Serial.println("Clearing WiFi credentials...");
    WiFi.disconnect(true);
    wm.resetSettings();
    delay(1000);
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
        return false;

    lastReconnectAttempt = now;

    if (isConnected())
        return true;

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

bool EspWiFiManager::disconnect()
{
    if (isConnected())
    {
        WiFi.disconnect();
        Serial.println("WiFi disconnected");
        return true;
    }
    return false;
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