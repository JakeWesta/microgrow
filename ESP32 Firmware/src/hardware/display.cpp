#include "display.h"
#include "../config/pins.h"
#include "../config/config.h"
#include <qrcode.h>

DisplayManager::DisplayManager()
    : tft(&SPI, PIN_TFT_CS, PIN_TFT_DC, PIN_TFT_RST), state(DisplayState::BOOT), wifiConnected(false), mqttConnected(false), lastTemp(0), lastHumidity(0), lastWaterLow(false)
{
}

bool DisplayManager::begin()
{
    pinMode(PIN_TFT_CS, OUTPUT);
    pinMode(PIN_TFT_DC, OUTPUT);
    pinMode(PIN_TFT_RST, OUTPUT);
    SPI.begin(SCK, -1, MOSI, 5);
    delay(10);
    tft.init(TFT_WIDTH, TFT_HEIGHT);
    delay(10);
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(ST77XX_BLACK);
    Serial.println("Display initialized");
    return true;
}

void DisplayManager::clear()
{
    tft.fillScreen(ST77XX_BLACK);
}

void DisplayManager::setBrightness(uint8_t level)
{
    // ST7789 doesn't have software brightness control
    // Would need external PWM on backlight pin
}

void DisplayManager::drawStatusBar()
{
    // Draw a simple status bar at top
    tft.fillRect(0, 0, TFT_WIDTH, 20, ST77XX_BLACK);
    tft.setTextSize(1);
    tft.setCursor(5, 5);

    // WiFi indicator
    tft.setTextColor(wifiConnected ? ST77XX_GREEN : ST77XX_RED);
    tft.print("WiFi ");

    // MQTT indicator
    tft.setTextColor(mqttConnected ? ST77XX_GREEN : ST77XX_RED);
    tft.print("MQTT");
}

void DisplayManager::drawHeader(const String &title, uint16_t color)
{
    clear();
    tft.setCursor(5, 30);
    tft.setTextColor(color);
    tft.setTextSize(4);
    tft.println(title);
}

void DisplayManager::showBoot()
{
    state = DisplayState::BOOT;
    drawHeader("MicroGrow", ST77XX_GREEN);

    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 100);
    tft.println("Initializing...");
}

void DisplayManager::showWiFiSetup(const String &deviceId)
{
    state = DisplayState::WIFI_SETUP;
    drawHeader("WiFi Setup", ST77XX_CYAN);

    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 90);
    tft.println("Connect to:");
    tft.setTextColor(ST77XX_YELLOW);
    tft.println("MicroGrow Setup");

    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 150);
    tft.println("Device ID:");
    tft.setTextSize(1);
    tft.setTextColor(ST77XX_CYAN);
    tft.println(deviceId);
}

void DisplayManager::showDeviceInfo(const String &id, const String &greenType)
{
    state = DisplayState::DEVICE_INFO;
    drawHeader("MicroGrow", ST77XX_GREEN);

    tft.setTextSize(3);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 90);
    tft.print("ID: ");
    tft.println(id);

    tft.setCursor(5, 130);
    tft.print("Type: ");
    tft.println(greenType);

    drawStatusBar();
}

void DisplayManager::showSensorData(float temp, float humidity, bool waterLow)
{
    // Only redraw if values changed
    if (state != DisplayState::RUNNING ||
        abs(temp - lastTemp) > 0.5 ||
        abs(humidity - lastHumidity) > 1.0 ||
        waterLow != lastWaterLow)
    {

        state = DisplayState::RUNNING;
        lastTemp = temp;
        lastHumidity = humidity;
        lastWaterLow = waterLow;

        drawStatusBar();

        // Clear data area
        tft.fillRect(0, 25, TFT_WIDTH, TFT_HEIGHT - 25, ST77XX_BLACK);

        tft.setTextSize(3);
        tft.setCursor(5, 50);

        // Temperature
        tft.setTextColor(ST77XX_YELLOW);
        tft.print("Temp: ");
        tft.setTextColor(ST77XX_WHITE);
        tft.print(temp, 1);
        tft.println(" F");

        // Humidity
        tft.setCursor(5, 90);
        tft.setTextColor(ST77XX_CYAN);
        tft.print("Humid: ");
        tft.setTextColor(ST77XX_WHITE);
        tft.print(humidity, 1);
        tft.println(" %");

        // Water level
        tft.setCursor(5, 130);
        tft.setTextColor(waterLow ? ST77XX_RED : ST77XX_GREEN);
        tft.print("Water: ");
        tft.println(waterLow ? "LOW" : "OK");
    }
}

void DisplayManager::showError(const String &message)
{
    state = DisplayState::ERROR;
    drawHeader("ERROR", ST77XX_RED);

    tft.setTextSize(2);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(5, 100);
    tft.println(message);
}

void DisplayManager::showQRCode(const String &data)
{
    clear();
    
    // Create QR code
    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(3)];
    
    // Initialize QR code
    qrcode_initText(&qrcode, qrcodeData, 3, ECC_LOW, data.c_str());
    
    // Calculate scaling to fit on screen (240x320)
    // Version 3 QR = 29x29 modules
    int scale = 6;  // 29 * 6 = 174 pixels
    int qrPixelSize = qrcode.size * scale;
    int offsetX = (TFT_WIDTH - qrPixelSize) / 2;   // Center horizontally
    int offsetY = 40;  // Leave room at top for title
    
    // Draw title
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_CYAN);
    tft.setCursor(5, 10);
    tft.println("Scan to Register");
    
    // Draw QR code
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            uint16_t color = qrcode_getModule(&qrcode, x, y) ? ST77XX_BLACK : ST77XX_WHITE;
            tft.fillRect(offsetX + (x * scale), 
                        offsetY + (y * scale), 
                        scale, scale, color);
        }
    }
}

void DisplayManager::setWiFiStatus(bool connected)
{
    if (wifiConnected != connected)
    {
        wifiConnected = connected;
        drawStatusBar();
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
