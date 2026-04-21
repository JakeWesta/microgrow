#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <functional>
#include <config/config.h>

class EspWiFiManager
{
public:
    EspWiFiManager();

    // Initialize and connect with saved credentials
    bool begin();

    // Start config portal, blocking internally but with a caller-supplied
    // abort predicate checked every 10 ms. Returns true if WiFi was saved
    // and connected successfully, false if aborted or timed out.
    bool runConfigPortal(std::function<bool()> shouldAbort = nullptr);

    // Clear saved credentials (call before runConfigPortal on a reset)
    void resetCredentials();

    // Connection management
    bool isConnected() const;
    bool reconnect();
    bool disconnect();

    // Device identification
    String getDeviceId() const { return "1"; }
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
};