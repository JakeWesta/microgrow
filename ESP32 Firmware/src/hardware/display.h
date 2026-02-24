#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ImageReader.h>
#include <SD.h>
#include "../config/pins.h"

enum class DisplayState
{
    BOOT,
    WIFI_SETUP,      // AP mode – waiting for user to join hotspot
    WIFI_CONNECTING, // WiFi joined – waiting for MQTT init/config to complete
    RUNNING,
    ERROR
};

class DisplayManager
{
public:
    DisplayManager();
    bool begin();
    void clear();
    void clearImg();
    void setBrightness(uint8_t level);

    // Screens
    void showBoot();
    void showWiFiSetup(const String &deviceId); // Call once; stores deviceId
    void showWiFiConnected();                   // Updates status line, keeps deviceId
    void showSensorData(float temp, float humidity, bool waterLow);
    void showError(const String &message);
    void showQRCode(const String &data);

    // Filename
    void setGreenType(const String &gt) { greenType = gt; }
    void setGrowth(const String &g) { growth = g; }
    String getFilename() const { return "/" + greenType + "-" + growth + "-" + String(animation_step) + ".bmp"; }

    // Status bar updates
    void setWiFiStatus(bool connected);
    void setMQTTStatus(bool connected);

    DisplayState getState() const { return state; }

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_RST};
    Adafruit_ImageReader reader;
    SdFat SD;
    uint8_t animation_step;
    uint32_t last_animation_ms;
    static constexpr uint32_t ANIMATION_FRAME_MS = 500; // frame rate
    static constexpr uint8_t ANIMATION_FRAMES = 1;

    DisplayState state;
    bool wifiConnected;
    bool mqttConnected;
    float lastTemp;
    float lastHumidity;
    bool lastWaterLow;
    String _deviceId;
    String greenType;
    String growth;

    // Drawing helpers
    void drawCenteredText(const String &text, int y, uint8_t size, uint16_t color);
    void drawRightText(const String &text, int y, uint8_t size, uint16_t color);
    void drawStatusBar();
    void drawPageHeader(const String &title, uint16_t color);
    void drawImage(const String &filename, int x, int y);

    // Internal WiFi screen renderer (phase 0 = AP, 1 = connected)
    void _renderWiFiSetup(uint8_t phase);
};