#include "display.h"

// ============================================================================
//  Color Palette
// ============================================================================
#define COLOR_BG ILI9341_BLACK
#define COLOR_SURFACE 0x1863
#define COLOR_ACCENT 0x07E0
#define COLOR_ACCENT_DIM 0x03E0
#define COLOR_CYAN 0x07FF
#define COLOR_GREEN_OK 0x07E0
#define COLOR_WARN 0xFFE0
#define COLOR_ERROR 0xF9A6
#define COLOR_WHITE ILI9341_WHITE
#define COLOR_BLACK ILI9341_BLACK
#define COLOR_GRAY 0xC618
#define COLOR_DARK_GRAY 0x4208

// Plant palette
#define COLOR_SOIL 0x4A28       // dark brown
#define COLOR_STEM 0x2D00       // medium green
#define COLOR_LEAF 0x0600       // bright green
#define COLOR_LEAF_DARK 0x0400  // dark green
#define COLOR_SEED_SHELL 0xC600 // tan/brown
#define COLOR_PETAL 0xF81F      // magenta/pink
#define COLOR_PETAL2 0xFD20     // orange
#define COLOR_STAMEN 0xFFE0     // yellow
#define COLOR_BERRY 0xF800      // red
#define COLOR_POT 0xC300        // terracotta

#define STATUS_H 26

// ============================================================================
//  Constructor
// ============================================================================

DisplayManager::DisplayManager()
    : state(DisplayState::BOOT),
      wifiConnected(false), mqttConnected(false),
      lastTemp(0), lastHumidity(0), lastWaterLow(false),
      _deviceId(""),
      greenType(""),
      growth(""),
      lastGrowth("")
{
}

// ============================================================================
//  Init
// ============================================================================

bool DisplayManager::begin()
{
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI);
    tft.begin();
    tft.setRotation(TFT_ROTATION);
    clear();
    return true;
}

void DisplayManager::clear()
{
    tft.fillScreen(COLOR_BG);
    yield();
}

// Clear only the plant/image area below the sensor strip
void DisplayManager::clearImg()
{
    tft.fillRect(0, 87, TFT_WIDTH, TFT_HEIGHT - 87, COLOR_BG);
    yield();
}

void DisplayManager::setBrightness(uint8_t level)
{
    // ILI9341 doesn't have software brightness control
}

// ============================================================================
//  Text helpers
// ============================================================================

static uint16_t measureTextWidth(const String &text, uint8_t size)
{
    return text.length() * 6 * size;
}

void DisplayManager::drawCenteredText(const String &text, int y, uint8_t size, uint16_t color)
{
    tft.setTextSize(size);
    tft.setTextColor(color, COLOR_BG);
    uint16_t textWidth = measureTextWidth(text, size);
    int16_t x = ((int16_t)TFT_WIDTH - (int16_t)textWidth) / 2;
    if (x < 0)
        x = 0;
    tft.setCursor(x, y);
    tft.print(text);
}

void DisplayManager::drawRightText(const String &text, int y, uint8_t size, uint16_t color)
{
    tft.setTextSize(size);
    tft.setTextColor(color, COLOR_BG);
    uint16_t textWidth = measureTextWidth(text, size);
    int16_t x = TFT_WIDTH - textWidth - 5;
    if (x < 0)
        x = 0;
    tft.setCursor(x, y);
    tft.print(text);
}

// ============================================================================
//  Status bar
// ============================================================================

void DisplayManager::drawStatusBar()
{
    tft.fillRect(0, 0, TFT_WIDTH, STATUS_H, COLOR_SURFACE);
    tft.drawFastHLine(0, STATUS_H, TFT_WIDTH, COLOR_ACCENT_DIM);
    yield();

    tft.setTextSize(2);

    uint16_t wifiColor = wifiConnected ? COLOR_GREEN_OK : COLOR_ERROR;
    tft.setTextColor(wifiColor, COLOR_SURFACE);
    tft.setCursor(4, 5);
    tft.print(wifiConnected ? "WiFi+" : "WiFi-");

    uint16_t mqttColor = mqttConnected ? COLOR_GREEN_OK : COLOR_ERROR;
    tft.setTextColor(mqttColor, COLOR_SURFACE);
    tft.setCursor(TFT_WIDTH - 66, 5);
    tft.print(mqttConnected ? "MQTT+" : "MQTT-");
}

void DisplayManager::drawPageHeader(const String &title, uint16_t color)
{
    drawCenteredText(title, 36, 3, color);
    uint16_t lineW = measureTextWidth(title, 3);
    int16_t lineX = (TFT_WIDTH - lineW) / 2;
    tft.drawFastHLine(lineX, 36 + 3 * 8 + 2, lineW, color);
}

// ============================================================================
//  Boot screen
// ============================================================================

void DisplayManager::showBoot()
{
    state = DisplayState::BOOT;
    clear();
    drawStatusBar();
    drawCenteredText("MicroGrow", 90, 5, COLOR_ACCENT);
    drawCenteredText("Smart Plant System", 138, 2, COLOR_CYAN);
    drawCenteredText("Initializing...", 175, 2, COLOR_ACCENT_DIM);
}

// ============================================================================
//  WiFi setup screen
// ============================================================================

void DisplayManager::showWiFiSetup(const String &deviceId, WiFiSetupReason reason)
{
    _deviceId = deviceId;

    switch (reason)
    {
    case WiFiSetupReason::NO_WIFI_NO_CONFIG:
        state = DisplayState::WIFI_SETUP_FRESH;
        break;
    case WiFiSetupReason::WIFI_NO_CONFIG:
        state = DisplayState::WIFI_SETUP_NO_CFG;
        break;
    case WiFiSetupReason::CONFIG_NO_WIFI:
        state = DisplayState::WIFI_SETUP_NO_WIFI;
        break;
    case WiFiSetupReason::PORTAL_WITH_CONFIG:
        state = DisplayState::WIFI_SETUP_PORTAL_CFG;
        break;
    }

    _renderWiFiSetup();
}

void DisplayManager::_renderWiFiSetup()
{
    yield();
    clear();
    drawStatusBar();

    if (state == DisplayState::WIFI_SETUP_NO_WIFI)
        drawCenteredText("Reconnecting...", 32, 3, COLOR_ACCENT);
    else
        drawCenteredText("Device Setup", 32, 3, COLOR_ACCENT);

    tft.drawFastHLine(10, 60, TFT_WIDTH - 20, COLOR_ACCENT_DIM);

    if (state == DisplayState::WIFI_SETUP_FRESH ||
        state == DisplayState::WIFI_SETUP_NO_CFG ||
        state == DisplayState::WIFI_SETUP_PORTAL_CFG)
    {
        drawCenteredText("Connect to:", 68, 2, COLOR_GRAY);
        drawCenteredText("MicroGrow Setup", 90, 2, COLOR_CYAN);
        tft.drawFastHLine(10, 118, TFT_WIDTH - 20, COLOR_DARK_GRAY);
        drawCenteredText("Device ID", 126, 2, COLOR_GRAY);

        uint16_t idW = measureTextWidth(_deviceId, 3);
        int16_t idX = (TFT_WIDTH - idW) / 2;
        tft.fillRect(idX - 8, 148, idW + 16, 26, COLOR_SURFACE);
        tft.drawRect(idX - 9, 147, idW + 18, 28, COLOR_ACCENT);
        tft.setTextColor(COLOR_ACCENT, COLOR_SURFACE);
        tft.setTextSize(3);
        tft.setCursor(idX, 151);
        tft.print(_deviceId);

        tft.drawFastHLine(10, 182, TFT_WIDTH - 20, COLOR_DARK_GRAY);
    }
    else // WIFI_SETUP_NO_WIFI
    {
        drawCenteredText("Device ID", 74, 2, COLOR_GRAY);

        uint16_t idW = measureTextWidth(_deviceId, 3);
        int16_t idX = (TFT_WIDTH - idW) / 2;
        tft.fillRect(idX - 8, 96, idW + 16, 26, COLOR_SURFACE);
        tft.drawRect(idX - 9, 95, idW + 18, 28, COLOR_ACCENT);
        tft.setTextColor(COLOR_ACCENT, COLOR_SURFACE);
        tft.setTextSize(3);
        tft.setCursor(idX, 99);
        tft.print(_deviceId);

        tft.drawFastHLine(10, 132, TFT_WIDTH - 20, COLOR_DARK_GRAY);
    }

    if (state == DisplayState::WIFI_SETUP_FRESH)
    {
        drawCenteredText("Open app to configure", 190, 2, COLOR_GRAY);
        drawCenteredText("after connecting", 212, 2, COLOR_GRAY);
    }
    else if (state == DisplayState::WIFI_SETUP_PORTAL_CFG)
    {
        drawCenteredText("Device already configured.", 200, 2, COLOR_GRAY);
    }
    else if (state == DisplayState::WIFI_SETUP_NO_CFG)
    {
        tft.fillRect(10, 188, TFT_WIDTH - 20, 28, COLOR_SURFACE);
        tft.drawRect(10, 188, TFT_WIDTH - 20, 28, COLOR_GREEN_OK);
        drawCenteredText("WiFi OK - Awaiting cfg", 194, 2, COLOR_GREEN_OK);
    }
    else // WIFI_SETUP_NO_WIFI
    {
        drawCenteredText("Previously configured.", 144, 2, COLOR_GRAY);
        drawCenteredText("Waiting for WiFi...", 166, 2, COLOR_GRAY);
    }
}

// ============================================================================
//  Running / sensor screen
// ============================================================================

void DisplayManager::showSensorData(float temp, float humidity, bool waterLow)
{
    bool firstRender = (state != DisplayState::RUNNING);
    state = DisplayState::RUNNING;
    lastTemp = temp;
    lastHumidity = humidity;
    lastWaterLow = waterLow;

    if (firstRender)
    {
        clear();
        drawStatusBar();
    }

    drawStatusBar();

    const int SENSOR_Y = STATUS_H + 15;
    const int DIVIDER_Y = STATUS_H + 15 + 40 + 4;

    // Redraw sensor strip
    tft.fillRect(0, STATUS_H + 1, TFT_WIDTH, DIVIDER_Y - STATUS_H + 2, COLOR_BG);
    yield();

    String tempStr = String(temp, 1) + " F";
    tft.setTextSize(4);
    tft.setTextColor(COLOR_GREEN_OK, COLOR_BG);
    tft.setCursor(4, SENSOR_Y);
    tft.print(tempStr);

    String humStr = String(humidity, 1) + " %";
    uint16_t humW = humStr.length() * 24;
    tft.setTextColor(COLOR_GREEN_OK, COLOR_BG);
    tft.setCursor(TFT_WIDTH - humW - 4, SENSOR_Y);
    tft.print(humStr);

    tft.fillRect(0, DIVIDER_Y, TFT_WIDTH, 2, COLOR_ACCENT_DIM);

    const int imgY = DIVIDER_Y + 2;
    const int plantH = TFT_HEIGHT - imgY;
    const int cx = TFT_WIDTH / 2;
    const int baseY = imgY + plantH - 80;

    // Redraw plant area on first render or whenever growth stage changes
    bool growthChanged = (growth != lastGrowth);
    if (firstRender || growthChanged)
    {
        tft.fillRect(0, imgY, TFT_WIDTH, plantH, COLOR_BG);
        if (growth.length() > 0)
        {
            drawPlant(cx, baseY);
            lastGrowth = growth;
        }
    }
}

// ============================================================================
//  Shutdown screen
// ============================================================================

void DisplayManager::showShutdown()
{
    state = DisplayState::SHUTDOWN;
    clear();

    tft.fillRect(0, 70, TFT_WIDTH, 100, 0x3000);
    tft.drawFastHLine(0, 70, TFT_WIDTH, COLOR_ERROR);
    tft.drawFastHLine(0, 170, TFT_WIDTH, COLOR_ERROR);

    drawCenteredText("Safe to Unplug", 90, 3, COLOR_WHITE);
    drawCenteredText("All systems off", 128, 2, COLOR_GRAY);
    drawCenteredText("MicroGrow", 195, 2, COLOR_ACCENT_DIM);
}

// ============================================================================
//  Error screen
// ============================================================================

void DisplayManager::showError(const String &message)
{
    state = DisplayState::ERROR;
    clear();
    drawStatusBar();

    tft.fillRect(0, STATUS_H + 1, TFT_WIDTH, 4, COLOR_ERROR);
    drawCenteredText("! ERROR !", 50, 4, COLOR_ERROR);
    tft.fillRect(0, 96, TFT_WIDTH, 4, COLOR_ERROR);
    drawCenteredText(message, 120, 2, COLOR_WHITE);
}

// ============================================================================
//  Status bar updates
// ============================================================================

void DisplayManager::setWiFiStatus(bool connected)
{
    if (state == DisplayState::SHUTDOWN)
        return;
    if (wifiConnected != connected)
    {
        wifiConnected = connected;
        drawStatusBar();
    }
}

void DisplayManager::setMQTTStatus(bool connected)
{
    if (state == DisplayState::SHUTDOWN)
        return;
    if (mqttConnected != connected)
    {
        mqttConnected = connected;
        drawStatusBar();
    }
}

// ============================================================================
//  Plant drawing – dispatcher
// ============================================================================

void DisplayManager::drawPlant(int cx, int baseY)
{
    Serial.printf("Drawing plant at growth stage '%s'\n", growth.c_str());
    if (growth == "seedling")
        drawPlantSeedling(cx, baseY);
    else if (growth == "vegetative")
        drawPlantVegetative(cx, baseY);
    else if (growth == "flowering")
        drawPlantFlowering(cx, baseY);
    else if (growth == "harvest")
        drawPlantHarvest(cx, baseY);
    else
        drawPlantSeedling(cx, baseY); // fallback
}

// ============================================================================
//  Stage 0 – SEEDLING
//  Soil mound with a cracked seed and two small cotyledons just emerging.
// ============================================================================
void DisplayManager::drawPlantSeedling(int cx, int baseY)
{
    // Soil mound
    tft.fillEllipse(cx, baseY - 8, 48, 16, COLOR_SOIL);

    // Stem (short)
    tft.drawLine(cx, baseY - 16, cx, baseY - 46, COLOR_STEM);
    tft.drawLine(cx + 1, baseY - 16, cx + 1, baseY - 46, COLOR_STEM);

    // Left cotyledon
    tft.fillEllipse(cx - 16, baseY - 47, 15, 8, COLOR_LEAF);
    tft.drawLine(cx, baseY - 44, cx - 16, baseY - 47, COLOR_LEAF_DARK);

    // Right cotyledon
    tft.fillEllipse(cx + 16, baseY - 47, 15, 8, COLOR_LEAF);
    tft.drawLine(cx, baseY - 44, cx + 16, baseY - 47, COLOR_LEAF_DARK);

    // Apical bud
    tft.fillCircle(cx, baseY - 54, 5, COLOR_LEAF);

    // Cracked seed shell visible at soil surface
    tft.fillEllipse(cx - 4, baseY - 14, 7, 5, COLOR_SEED_SHELL);
    tft.fillEllipse(cx + 6, baseY - 13, 6, 4, COLOR_SEED_SHELL);

    drawCenteredText("Seedling", baseY - 76, 2, COLOR_GRAY);
}

// ============================================================================
//  Stage 1 – VEGETATIVE
//  Taller stem with alternating leaves.
// ============================================================================
void DisplayManager::drawPlantVegetative(int cx, int baseY)
{
    // Pot body (trapezoid approximation with rects + triangles via lines)
    tft.fillRect(cx - 22, baseY - 26, 44, 24, COLOR_POT);
    tft.fillTriangle(cx - 22, baseY - 26, cx - 30, baseY - 2, cx - 22, baseY - 2, COLOR_POT);
    tft.fillTriangle(cx + 22, baseY - 26, cx + 30, baseY - 2, cx + 22, baseY - 2, COLOR_POT);
    // Pot rim
    tft.fillRect(cx - 28, baseY - 30, 56, 6, COLOR_GRAY);
    // Soil in pot
    tft.fillEllipse(cx, baseY - 28, 22, 6, COLOR_SOIL);

    // Main stem
    int stemTop = baseY - 100;
    for (int i = 0; i < 2; i++)
        tft.drawLine(cx + i, baseY - 28, cx + i, stemTop, COLOR_STEM);

    // Leaf pair 1 (low)
    tft.fillEllipse(cx - 22, baseY - 60, 20, 9, COLOR_LEAF);
    tft.drawLine(cx, baseY - 58, cx - 22, baseY - 60, COLOR_LEAF_DARK);
    tft.fillEllipse(cx + 22, baseY - 70, 20, 9, COLOR_LEAF);
    tft.drawLine(cx, baseY - 68, cx + 22, baseY - 70, COLOR_LEAF_DARK);

    // Leaf pair 2 (mid)
    tft.fillEllipse(cx - 20, baseY - 88, 18, 8, COLOR_LEAF);
    tft.drawLine(cx, baseY - 86, cx - 20, baseY - 88, COLOR_LEAF_DARK);
    tft.fillEllipse(cx + 20, baseY - 96, 18, 8, COLOR_LEAF);
    tft.drawLine(cx, baseY - 94, cx + 20, baseY - 96, COLOR_LEAF_DARK);

    // Apical bud
    tft.fillCircle(cx, stemTop - 4, 6, COLOR_LEAF);

    drawCenteredText("Vegetative", baseY - 120, 2, COLOR_GRAY);
}

// ============================================================================
//  Stage 2 – FLOWERING
//  Full plant with flower at the top (4 petals + stamen).
// ============================================================================
void DisplayManager::drawPlantFlowering(int cx, int baseY)
{
    // Pot
    tft.fillRect(cx - 22, baseY - 26, 44, 24, COLOR_POT);
    tft.fillTriangle(cx - 22, baseY - 26, cx - 30, baseY - 2, cx - 22, baseY - 2, COLOR_POT);
    tft.fillTriangle(cx + 22, baseY - 26, cx + 30, baseY - 2, cx + 22, baseY - 2, COLOR_POT);
    tft.fillRect(cx - 28, baseY - 30, 56, 6, COLOR_GRAY);
    tft.fillEllipse(cx, baseY - 28, 22, 6, COLOR_SOIL);

    // Stem
    int stemTop = baseY - 110;
    for (int i = 0; i < 2; i++)
        tft.drawLine(cx + i, baseY - 28, cx + i, stemTop, COLOR_STEM);

    // Leaves
    tft.fillEllipse(cx - 22, baseY - 60, 20, 9, COLOR_LEAF);
    tft.drawLine(cx, baseY - 58, cx - 22, baseY - 60, COLOR_LEAF_DARK);
    tft.fillEllipse(cx + 22, baseY - 72, 20, 9, COLOR_LEAF);
    tft.drawLine(cx, baseY - 70, cx + 22, baseY - 72, COLOR_LEAF_DARK);
    tft.fillEllipse(cx - 18, baseY - 90, 16, 7, COLOR_LEAF);
    tft.drawLine(cx, baseY - 88, cx - 18, baseY - 90, COLOR_LEAF_DARK);

    // Flower – 4 petals (N/S/E/W ellipses)
    int fx = cx, fy = stemTop - 14;
    tft.fillEllipse(fx, fy - 14, 8, 14, COLOR_PETAL);  // top
    tft.fillEllipse(fx, fy + 14, 8, 14, COLOR_PETAL);  // bottom
    tft.fillEllipse(fx - 14, fy, 14, 8, COLOR_PETAL2); // left
    tft.fillEllipse(fx + 14, fy, 14, 8, COLOR_PETAL2); // right
    // Stamen centre
    tft.fillCircle(fx, fy, 9, COLOR_STAMEN);
    tft.fillCircle(fx, fy, 5, COLOR_BG); // hollow centre

    drawCenteredText("Flowering", baseY - 130, 2, COLOR_GRAY);
}

// ============================================================================
//  Stage 3 – HARVEST
//  Plant with fruit/berries visible.
// ============================================================================
void DisplayManager::drawPlantHarvest(int cx, int baseY)
{
    // Pot
    tft.fillRect(cx - 22, baseY - 26, 44, 24, COLOR_POT);
    tft.fillTriangle(cx - 22, baseY - 26, cx - 30, baseY - 2, cx - 22, baseY - 2, COLOR_POT);
    tft.fillTriangle(cx + 22, baseY - 26, cx + 30, baseY - 2, cx + 22, baseY - 2, COLOR_POT);
    tft.fillRect(cx - 28, baseY - 30, 56, 6, COLOR_GRAY);
    tft.fillEllipse(cx, baseY - 28, 22, 6, COLOR_SOIL);

    // Stem (slightly shorter – heavy with fruit)
    int stemTop = baseY - 106;
    for (int i = 0; i < 2; i++)
        tft.drawLine(cx + i, baseY - 28, cx + i, stemTop, COLOR_STEM);

    // Leaves
    tft.fillEllipse(cx - 24, baseY - 58, 22, 10, COLOR_LEAF);
    tft.drawLine(cx, baseY - 56, cx - 24, baseY - 58, COLOR_LEAF_DARK);
    tft.fillEllipse(cx + 24, baseY - 70, 22, 10, COLOR_LEAF);
    tft.drawLine(cx, baseY - 68, cx + 24, baseY - 70, COLOR_LEAF_DARK);
    tft.fillEllipse(cx - 18, baseY - 90, 16, 7, COLOR_LEAF);
    tft.drawLine(cx, baseY - 88, cx - 18, baseY - 90, COLOR_LEAF_DARK);

    // Canopy blob
    tft.fillCircle(cx, stemTop - 4, 20, COLOR_LEAF_DARK);
    tft.fillCircle(cx - 14, stemTop, 14, COLOR_LEAF);
    tft.fillCircle(cx + 14, stemTop, 14, COLOR_LEAF);
    tft.fillCircle(cx, stemTop - 16, 14, COLOR_LEAF);

    // Berries / fruit
    tft.fillCircle(cx - 10, stemTop - 4, 6, COLOR_BERRY);
    tft.fillCircle(cx + 12, stemTop - 2, 6, COLOR_BERRY);
    tft.fillCircle(cx, stemTop - 18, 5, COLOR_BERRY);
    // Specular highlights
    tft.fillCircle(cx - 12, stemTop - 6, 2, COLOR_WHITE);
    tft.fillCircle(cx + 10, stemTop - 4, 2, COLOR_WHITE);
    tft.fillCircle(cx - 2, stemTop - 20, 2, COLOR_WHITE);

    drawCenteredText("Harvest!", baseY - 130, 2, COLOR_STAMEN);
}