#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_ImageReader.h>
#include <SD.h>
#include "../config/pins.h"

// Why the WiFi setup screen is being shown - drives which variant renders
enum class WiFiSetupReason
{
    NO_WIFI_NO_CONFIG,  // Fresh device: no credentials, no config
    WIFI_NO_CONFIG,     // WiFi connected but no DeviceConfig yet
    CONFIG_NO_WIFI,     // DeviceConfig exists but WiFi is down
    PORTAL_WITH_CONFIG, // User requested portal, but config exists
};

enum class DisplayState
{
    BOOT,
    WIFI_SETUP_FRESH,      // NO_WIFI_NO_CONFIG
    WIFI_SETUP_NO_CFG,     // WIFI_NO_CONFIG
    WIFI_SETUP_NO_WIFI,    // CONFIG_NO_WIFI
    WIFI_SETUP_PORTAL_CFG, // PORTAL_WITH_CONFIG
    RUNNING,
    ERROR,
    SHUTDOWN,
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

    // Unified WiFi setup screen - renders based on reason
    void showWiFiSetup(const String &deviceId, WiFiSetupReason reason);

    void showSensorData(float temp, float humidity, bool waterLow);
    void showError(const String &message);
    void showQRCode(const String &data);
    void showShutdown();

    // Image / animation
    void setGreenType(const String &gt) { greenType = gt; }
    void setGrowth(const String &g) { growth = g; }
    String getFilename() const
    {
        return "/" + greenType + "-" + growth + "-" + String(animation_step) + ".bmp";
    }

    // Status bar updates - ONLY redraws the status bar, never changes screen
    void setWiFiStatus(bool connected);
    void setMQTTStatus(bool connected);

    DisplayState getState() const { return state; }

    void drawImage(const String &filename, int x, int y);

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_RST};
    Adafruit_ImageReader reader;
    SdFat SD;
    uint8_t animation_step;
    uint32_t last_animation_ms;
    static constexpr uint32_t ANIMATION_FRAME_MS = 500;
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
    void _renderWiFiSetup();
};