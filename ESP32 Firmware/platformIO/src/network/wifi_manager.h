#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>

class EspWiFiManager
{
public:
    EspWiFiManager();

    // Initialize and connect
    bool begin();

    // Start configuration portal
    bool startConfigPortal();

    // Clear saved credentials and restart portal
    void resetCredentials();

    // Connection management
    bool isConnected() const;
    bool reconnect();

    // Device identification
    String getDeviceId() const { return deviceId; }
    String getMacAddress() const;
    String getIPAddress() const;

    // Status
    void printStatus() const;

private:
    String deviceId;
    WiFiManager wm;
    bool configured;
    uint32_t lastReconnectAttempt;

    void generateDeviceId();
    void setupPortalCallbacks();
};
