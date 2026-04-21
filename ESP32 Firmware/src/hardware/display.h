#pragma once

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <config/pins.h>
#include <config/config.h>

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
    void showShutdown();

    // Plant illustration
    void setGreenType(const String &gt) { greenType = gt; }
    void setGrowth(const String &g) { growth = g; }

    // Status bar updates - ONLY redraws the status bar, never changes screen
    void setWiFiStatus(bool connected);
    void setMQTTStatus(bool connected);

    DisplayState getState() const { return state; }

private:
    Adafruit_ILI9341 tft{TFT_CS, TFT_DC, TFT_RST};

    DisplayState state;
    bool wifiConnected;
    bool mqttConnected;
    float lastTemp;
    float lastHumidity;
    bool lastWaterLow;
    String _deviceId;
    String greenType;
    String growth;
    String lastGrowth; // tracks last drawn stage so we redraw on change

    // Drawing helpers
    void drawCenteredText(const String &text, int y, uint8_t size, uint16_t color);
    void drawRightText(const String &text, int y, uint8_t size, uint16_t color);
    void drawStatusBar();
    void drawPageHeader(const String &title, uint16_t color);
    void _renderWiFiSetup();

    // Plant illustration – one method per stage (0–3)
    void drawPlant(int cx, int baseY);
    void drawPlantSeedling(int cx, int baseY);   // 0
    void drawPlantVegetative(int cx, int baseY); // 1
    void drawPlantFlowering(int cx, int baseY);  // 2
    void drawPlantHarvest(int cx, int baseY);    // 3
};