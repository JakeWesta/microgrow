#pragma once

#include <Adafruit_ST7789.h>
#include <Arduino.h>

enum class DisplayState
{
    BOOT,
    WIFI_SETUP,
    DEVICE_INFO,
    RUNNING,
    ERROR
};

class DisplayManager
{
public:
    DisplayManager();

    bool begin();

    // Screen updates
    void showBoot();
    void showWiFiSetup(const String &deviceId);
    void showDeviceInfo(const String &habitatId, const String &greenType);
    void showQRCode(const String &data); // For easy pairing
    void showSensorData(float temp, float humidity, bool waterLow);
    void showError(const String &message);

    // Status indicators
    void setWiFiStatus(bool connected);
    void setMQTTStatus(bool connected);

    void clear();
    void setBrightness(uint8_t level); // 0-255

private:
    Adafruit_ST7789 tft;
    DisplayState state;
    bool wifiConnected;
    bool mqttConnected;

    // Last sensor values to avoid unnecessary redraws
    float lastTemp;
    float lastHumidity;
    bool lastWaterLow;

    void drawStatusBar();
    void drawHeader(const String &title, uint16_t color);
};
