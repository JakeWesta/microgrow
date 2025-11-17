#include <DHT20.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GFX.h>
#include <Adafruit_GrayOLED.h>
#include <Adafruit_SPITFT.h>
#include <Adafruit_SPITFT_Macros.h>
#include <gfxfont.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_ST7796S.h>
#include <Adafruit_ST77xx.h>
#include <FastLED.h>
#include "defs.h"
#include <Preferences.h>
#include <SPI.h>

// Forward declaration of tasks
void mqttTask(void *);
void actuatorTask(void *);
void sensorTask(void *);
void scheduleTask(void *);

// Forward declaration of sensor/actuator functions
float readTemperature(void);
float readHumidity(void);
float readWaterLevel(void);
void ledOn(void);
void ledOff(void);
void writeFan(void *);
void pumpOn(void);
void pumpOff(void);

Shared shared;
SemaphoreHandle_t mutex;
bool initialized;
Preferences prefs;
void IRAM_ATTR handleButton(); // ISR prototype
volatile bool portalRequested = false;
CRGB leds[NUM_LEDS];
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
DHT20 dht20;
uint8_t fan_val;

void initHardware() {
    // Button interrupt for config portal
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButton, FALLING);

    // Motor
    pinMode(MOTOR_PIN, OUTPUT);
    digitalWrite(MOTOR_PIN, LOW);
    shared.schedules[WATER_ID_S].actionStart = pumpOn;
    shared.schedules[WATER_ID_S].actionEnd = pumpOff;

    // Fan
    pinMode(FAN_PIN, OUTPUT);
    digitalWrite(FAN_PIN, LOW);
    shared.actuators[FAN_ID] = {.writeFunc = writeFan, .cmdData = &fan_val, .manualOverride = false, .manualTriggered = false};

    // Water-level sensor
    pinMode(WATER_LEVEL_PIN, INPUT);
    shared.sensors[WATER_LEVEL_ID].readFunc = readWaterLevel;

    // LEDs
    FastLED.addLeds<WS2812, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(50);
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    shared.schedules[LED_ID_S].actionStart = ledOn;
    shared.schedules[LED_ID_S].actionEnd = ledOff;


    // TFT Display
    tft.init(240, 320);
    tft.fillScreen(ST77XX_BLACK);

    // Temperature and humidity sensors
    Wire.begin();
    dht20.begin();
    shared.sensors[TEMPERATURE_ID].readFunc = readTemperature;
    shared.sensors[HUMIDITY_ID].readFunc = readHumidity;
}


void setup() {
  Serial.begin(115200);
  delay(1000);

  initHardware();

  // Create mutex
  mutex = xSemaphoreCreateMutex();

  prefs.begin("microgrow", true);

  initialized = prefs.getBool("init", false);

  if (!initialized) {
      Serial.println("NVS empty");

      shared.targets_ready = false;
      shared.schedules_ready = false;

  } else {
      Serial.println("NVS loaded");

      shared.targets.humidity    = prefs.getFloat("tHumidity", 60.0f);
      shared.targets.temperature = prefs.getFloat("tTemp", 75.0f);

      shared.schedules[LED_ID_S].startSec   = prefs.getULong("light_start", 0);
      shared.schedules[LED_ID_S].durationSec    = prefs.getULong("light_dur",   0);
      shared.schedules[LED_ID_S].intervalSec    = prefs.getULong("light_int",   0);

      shared.schedules[WATER_ID_S].startSec   = prefs.getULong("water_start", 0);
      shared.schedules[WATER_ID_S].durationSec    = prefs.getULong("water_dur",   0);
      shared.schedules[WATER_ID_S].intervalSec    = prefs.getULong("water_int",   0);

      shared.targets_ready = true;
      shared.schedules_ready = true;
  }

  prefs.end();

  // Create tasks
  xTaskCreatePinnedToCore(sensorTask, "Sensor", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(mqttTask, "MQTT", 8192, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(actuatorTask, "Actuator", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(scheduleTask, "Schedule", 4096, NULL, 1, NULL, 1);
}

void loop() {
  vTaskDelay(portMAX_DELAY);  // nothing runs here
}

// ISR sets a flag to trigger the portal
void IRAM_ATTR handleButton() {
  portalRequested = true;
}
