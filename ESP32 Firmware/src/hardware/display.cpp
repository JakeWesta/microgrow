#include "display.h"
#include "../config/pins.h"
#include "../config/config.h"
#include <qrcode.h>
#include <SPI.h>

//  Color Palette
// Black background, vivid neon accents
#define COLOR_BG ILI9341_BLACK  // 0xF2FFF3
#define COLOR_SURFACE 0x1863    // Dark charcoal (status bar)
#define COLOR_ACCENT 0x07E0     // Electric green  #00FF00
#define COLOR_ACCENT_DIM 0x03E0 // Mid green       #007800  (labels)
#define COLOR_CYAN 0x07FF       // Bright cyan     #00FFFF
#define COLOR_GREEN_OK 0x07E0   // Electric green  (water OK)
#define COLOR_WARN 0xFFE0       // Bright yellow   #FFFF00  (water LOW)
#define COLOR_ERROR 0xF9A6      // Hot coral red
#define COLOR_WHITE ILI9341_WHITE
#define COLOR_BLACK ILI9341_BLACK
#define COLOR_GRAY 0xC618      // Light gray      (secondary text)
#define COLOR_DARK_GRAY 0x4208 // Dim gray        (dividers)

// Status bar height
#define STATUS_H 26

DisplayManager::DisplayManager()
    : state(DisplayState::BOOT),
      wifiConnected(false), mqttConnected(false),
      lastTemp(0), lastHumidity(0), lastWaterLow(false),
      _deviceId(""),
      greenType(""),
      growth("seed"),
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

//  Text helpers

/**
 * Returns the pixel width of `text` at the given text size.
 * Each character in the Adafruit GFX default font is 6px wide (5px + 1 gap),
 * scaled by `size`. This avoids getTextBounds quirks with y-offsets.
 */
static uint16_t measureTextWidth(const String &text, uint8_t size)
{
    return text.length() * 6 * size;
}

void DisplayManager::drawCenteredText(const String &text, int y, uint8_t size, uint16_t color)
{
    tft.setTextSize(size);
    tft.setTextColor(color, COLOR_BG); // opaque background to prevent ghost text

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

//  Status bar

void DisplayManager::drawStatusBar()
{
    // Gradient-like effect: solid bar with a thin accent line below
    tft.fillRect(0, 0, TFT_WIDTH, STATUS_H, COLOR_SURFACE);
    tft.drawFastHLine(0, STATUS_H, TFT_WIDTH, COLOR_ACCENT_DIM);
    yield();

    tft.setTextSize(2);

    // WiFi indicator – left
    uint16_t wifiColor = wifiConnected ? COLOR_GREEN_OK : COLOR_ERROR;
    tft.setTextColor(wifiColor, COLOR_SURFACE);
    tft.setCursor(4, 5);
    tft.print(wifiConnected ? "WiFi+" : "WiFi-");

    // MQTT indicator – right
    uint16_t mqttColor = mqttConnected ? COLOR_GREEN_OK : COLOR_ERROR;
    tft.setTextColor(mqttColor, COLOR_SURFACE);
    tft.setCursor(TFT_WIDTH - 66, 5);
    tft.print(mqttConnected ? "MQTT+" : "MQTT-");
}

//  Section header (reusable)

void DisplayManager::drawPageHeader(const String &title, uint16_t color)
{
    // Title centered around y=36 (below status bar)
    drawCenteredText(title, 36, 3, color);
    // Decorative underline
    uint16_t lineW = measureTextWidth(title, 3);
    int16_t lineX = (TFT_WIDTH - lineW) / 2;
    tft.drawFastHLine(lineX, 36 + 3 * 8 + 2, lineW, color);
}

//  Boot screen

void DisplayManager::showBoot()
{
    state = DisplayState::BOOT;
    clear();
    drawStatusBar();

    // Logo / wordmark
    drawCenteredText("MicroGrow", 90, 5, COLOR_ACCENT);

    // Tagline
    drawCenteredText("Smart Plant System", 138, 2, COLOR_CYAN);

    // Status
    drawCenteredText("Initializing...", 175, 2, COLOR_ACCENT_DIM);
}

//  WiFi setup / connecting screens

/**
 * Internal helper: renders the WiFi setup layout.
 * `phase` controls the status line:
 *   0 = "Connect to hotspot below"  (AP not yet joined)
 *   1 = "WiFi connected! Awaiting setup..." (AP joined, config in progress)
 */
void DisplayManager::_renderWiFiSetup(uint8_t phase)
{
    clear();
    drawStatusBar();

    //  Header
    drawCenteredText("Device Setup", 32, 3, COLOR_ACCENT);
    tft.drawFastHLine(10, 60, TFT_WIDTH - 20, COLOR_ACCENT_DIM);

    //  Hotspot section
    drawCenteredText("Connect to:", 68, 2, COLOR_GRAY);
    drawCenteredText("MicroGrow-Setup", 90, 2, COLOR_CYAN);

    tft.drawFastHLine(10, 118, TFT_WIDTH - 20, COLOR_DARK_GRAY);

    //  Device ID section
    drawCenteredText("Device ID", 126, 2, COLOR_GRAY);

    // Highlight box for device ID
    uint16_t idW = measureTextWidth(_deviceId, 3);
    int16_t idX = (TFT_WIDTH - idW) / 2;
    tft.fillRect(idX - 8, 148, idW + 16, 26, COLOR_SURFACE);
    tft.drawRect(idX - 9, 147, idW + 18, 28, COLOR_ACCENT);
    tft.setTextColor(COLOR_ACCENT, COLOR_SURFACE);
    tft.setTextSize(3);
    tft.setCursor(idX, 151);
    tft.print(_deviceId);

    tft.drawFastHLine(10, 182, TFT_WIDTH - 20, COLOR_DARK_GRAY);

    //  Status line
    if (phase == 0)
    {
        drawCenteredText("Open app to configure", 190, 2, COLOR_GRAY);
        drawCenteredText("after connecting", 212, 2, COLOR_GRAY);
    }
    else
    {
        tft.fillRect(10, 188, TFT_WIDTH - 20, 28, COLOR_SURFACE);
        tft.drawRect(10, 188, TFT_WIDTH - 20, 28, COLOR_GREEN_OK);
        drawCenteredText("WiFi OK - Awaiting cfg", 194, 2, COLOR_GREEN_OK);
    }
}

void DisplayManager::showWiFiSetup(const String &deviceId)
{
    _deviceId = deviceId;
    state = DisplayState::WIFI_SETUP;
    _renderWiFiSetup(0);
}

void DisplayManager::showWiFiConnected()
{
    // Keep device ID on screen, just update status line
    state = DisplayState::WIFI_CONNECTING;
    _renderWiFiSetup(1);
    drawStatusBar(); // refresh status bar to show WiFi green
}

//  Running / sensor screen

void DisplayManager::showSensorData(float temp, float humidity, bool waterLow)
{
    if (state != DisplayState::RUNNING)
    {
        clear();
        drawStatusBar();
    }

    // bool changed = (state != DisplayState::RUNNING) ||
    //                (fabsf(temp - lastTemp) > 0.5f) ||
    //                (fabsf(humidity - lastHumidity) > 1.0f);

    // if (!changed)
    //     return;

    state = DisplayState::RUNNING;
    lastTemp = temp;
    lastHumidity = humidity;
    lastWaterLow = waterLow;

    drawStatusBar();

    const int SENSOR_Y = STATUS_H + 15;           // y=41
    const int DIVIDER_Y = STATUS_H + 15 + 40 + 4; // y=85  (32px text + padding)

    // Clear sensor row
    tft.fillRect(0, STATUS_H + 1, TFT_WIDTH, DIVIDER_Y - STATUS_H + 2, COLOR_BG);
    yield();

    //  Temperature — top left
    // size 4 = 24px wide × 32px tall per char
    String tempStr = String(temp, 1) + " F";
    tft.setTextSize(4);
    tft.setTextColor(COLOR_GREEN_OK, COLOR_BG);
    tft.setCursor(4, SENSOR_Y);
    tft.print(tempStr);

    //  Humidity — top right
    String humStr = String(humidity, 1) + " %";
    uint16_t humW = humStr.length() * 24; // 24px per char at size 4
    tft.setTextColor(COLOR_GREEN_OK, COLOR_BG);
    tft.setCursor(TFT_WIDTH - humW - 4, SENSOR_Y);
    tft.print(humStr);

    //  Divider
    tft.fillRect(0, DIVIDER_Y, TFT_WIDTH, 2, COLOR_ACCENT_DIM);

    //  Plant image — scaled to fill everything below divider
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

//  Error screen

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

//  QR Code screen

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

    // White quiet zone
    tft.fillRect(offsetX - 6, offsetY - 6, qrPixelSize + 12, qrPixelSize + 12, COLOR_WHITE);
    tft.drawRect(offsetX - 7, offsetY - 7, qrPixelSize + 14, qrPixelSize + 14, COLOR_ACCENT);

    for (uint8_t y = 0; y < qrcode.size; y++)
    {
        for (uint8_t x = 0; x < qrcode.size; x++)
        {
            uint16_t color = qrcode_getModule(&qrcode, x, y) ? COLOR_BLACK : COLOR_WHITE;
            tft.fillRect(offsetX + (x * scale), offsetY + (y * scale), scale, scale, color);
        }
    }
}

//  Status helpers

void DisplayManager::setWiFiStatus(bool connected)
{
    if (wifiConnected != connected)
    {
        wifiConnected = connected;

        // If we're on the setup screen and WiFi just connected, upgrade the view
        if (connected && (state == DisplayState::WIFI_SETUP || state == DisplayState::WIFI_CONNECTING))
        {
            showWiFiConnected();
        }
        else
        {
            drawStatusBar();
        }
    }
}

void DisplayManager::setMQTTStatus(bool connected)
{
    if (mqttConnected != connected)
    {
        mqttConnected = connected;
        drawStatusBar();
    }
}

//  Image helper

void DisplayManager::drawImage(const String &filename, int x, int y)
{
    ImageReturnCode stat = reader.drawBMP(filename.c_str(), tft, x, y);
}