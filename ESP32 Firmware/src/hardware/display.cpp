#include "display.h"
#include "../config/pins.h"
#include "../config/config.h"
#include <qrcode.h>
#include <SPI.h>

//  Color Palette
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

#define STATUS_H 26

DisplayManager::DisplayManager()
    : state(DisplayState::BOOT),
      wifiConnected(false), mqttConnected(false),
      lastTemp(0), lastHumidity(0), lastWaterLow(false),
      _deviceId(""),
      greenType(""),
      growth(""),
      reader(SD),
      animation_step(0),
      last_animation_ms(0)
{
}

bool DisplayManager::begin()
{
    SPI.begin(TFT_SCLK, TFT_MISO, TFT_MOSI, TFT_CS);
    SD.begin(SD_CS, SD_SCK_MHZ(12));
    delay(10);
    tft.begin();
    delay(10);
    tft.setRotation(TFT_ROTATION);
    clear();
    return true;
}

void DisplayManager::clear()
{
    tft.fillScreen(COLOR_BG);
    yield();
}

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
// Text helpers
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
// Status bar
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
// Boot screen
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
// WiFi setup screen
//
// Renders differently based on current DisplayState:
//
//   WIFI_SETUP_FRESH   - no WiFi, no config: "Connect to hotspot, open app"
//   WIFI_SETUP_NO_CFG  - WiFi connected, no config: "WiFi OK, awaiting config"
//   WIFI_SETUP_NO_WIFI - config exists, no WiFi: "Reconnecting..."
//  WIFI_SETUP_PORTAL_CFG - user requested portal, config exists: "Previously configured, waiting for WiFi"
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
    clear();
    drawStatusBar();

    //  Shared: header + device ID box

    if (state == DisplayState::WIFI_SETUP_NO_WIFI)
    {
        drawCenteredText("Reconnecting...", 32, 3, COLOR_ACCENT);
    }
    else
    {
        drawCenteredText("Device Setup", 32, 3, COLOR_ACCENT);
    }
    tft.drawFastHLine(10, 60, TFT_WIDTH - 20, COLOR_ACCENT_DIM);

    if (state == DisplayState::WIFI_SETUP_FRESH ||
        state == DisplayState::WIFI_SETUP_NO_CFG ||
        state == DisplayState::WIFI_SETUP_PORTAL_CFG) // <-- Added this state
    {
        // Show hotspot SSID to connect to
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
        // Show device ID higher up since there's no hotspot instruction
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

    //  Bottom status line – differs per reason

    if (state == DisplayState::WIFI_SETUP_FRESH)
    {
        // Not connected, no config - instruct user to connect and open app
        drawCenteredText("Open app to configure", 190, 2, COLOR_GRAY);
        drawCenteredText("after connecting", 212, 2, COLOR_GRAY);
    }
    else if (state == DisplayState::WIFI_SETUP_PORTAL_CFG)
    {
        drawCenteredText("Device already configured.", 190, 2, COLOR_GRAY);
        drawCenteredText("Update WiFi if needed.", 212, 2, COLOR_GRAY);
    }
    else if (state == DisplayState::WIFI_SETUP_NO_CFG)
    {
        // WiFi connected, waiting for MQTT to deliver config
        tft.fillRect(10, 188, TFT_WIDTH - 20, 28, COLOR_SURFACE);
        tft.drawRect(10, 188, TFT_WIDTH - 20, 28, COLOR_GREEN_OK);
        drawCenteredText("WiFi OK - Awaiting cfg", 194, 2, COLOR_GREEN_OK);
    }
    else // WIFI_SETUP_NO_WIFI
    {
        // Config exists but WiFi is down
        drawCenteredText("Previously configured.", 144, 2, COLOR_GRAY);
        drawCenteredText("Waiting for WiFi...", 166, 2, COLOR_GRAY);
    }
}

// ============================================================================
// Running / sensor screen
// ============================================================================

void DisplayManager::showSensorData(float temp, float humidity, bool waterLow)
{
    if (state != DisplayState::RUNNING)
    {
        clear();
        drawStatusBar();
    }

    state = DisplayState::RUNNING;
    lastTemp = temp;
    lastHumidity = humidity;
    lastWaterLow = waterLow;

    drawStatusBar();

    const int SENSOR_Y = STATUS_H + 15;
    const int DIVIDER_Y = STATUS_H + 15 + 40 + 4;

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
    uint32_t now = millis();
    if (now - last_animation_ms >= ANIMATION_FRAME_MS)
    {
        animation_step = (animation_step + 1) % ANIMATION_FRAMES;
        last_animation_ms = now;
        int imgWidth = 0, imgHeight = 0;
        String filename = getFilename();
        reader.bmpDimensions(filename.c_str(), &imgWidth, &imgHeight);
        int imgX = (TFT_WIDTH - imgWidth) / 2;
        drawImage(filename.c_str(), imgX, imgY);
    }
}

// ============================================================================
// Shutdown screen
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
// Error screen
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
// QR code screen
// ============================================================================

void DisplayManager::showQRCode(const String &data)
{
    clear();

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, data.c_str());

    int scale = 6;
    int qrPixelSize = qrcode.size * scale;
    int offsetX = (TFT_WIDTH - qrPixelSize) / 2;
    int offsetY = 50;

    drawCenteredText("Scan to Register", 12, 2, COLOR_CYAN);

    tft.fillRect(offsetX - 6, offsetY - 6, qrPixelSize + 12, qrPixelSize + 12, COLOR_WHITE);
    tft.drawRect(offsetX - 7, offsetY - 7, qrPixelSize + 14, qrPixelSize + 14, COLOR_ACCENT);

    for (uint8_t y = 0; y < qrcode.size; y++)
        for (uint8_t x = 0; x < qrcode.size; x++)
        {
            uint16_t color = qrcode_getModule(&qrcode, x, y) ? COLOR_BLACK : COLOR_WHITE;
            tft.fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, color);
        }
}

// ============================================================================
// Status bar updates
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
// Image helper
// ============================================================================

void DisplayManager::drawImage(const String &filename, int x, int y)
{
    reader.drawBMP(filename.c_str(), tft, x, y);
}