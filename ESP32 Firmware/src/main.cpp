#include <Arduino.h>
#include "config/pins.h"
#include "config/config.h"
#include "hardware/sensors.h"
#include "hardware/actuators.h"
#include "hardware/display.h"
#include "control/scheduler.h"
#include "control/automation.h"
#include "network/wifi_manager.h"
#include "network/mqtt_client.h"
#include "storage/persistence.h"

// Global pointers - initialized in setup()
SensorManager *sensors = nullptr;
ActuatorManager *actuators = nullptr;
DisplayManager *display = nullptr;
Scheduler *scheduler = nullptr;
AutomationController *automation = nullptr;
EspWiFiManager *wifiMgr = nullptr;
PersistenceManager *storage = nullptr;
MQTTClient *mqttClient = nullptr;

// ISR for WiFi reset button
volatile bool resetWiFiRequested = false;

void IRAM_ATTR handleResetButton()
{
    resetWiFiRequested = true;
}

// FreeRTOS Tasks
void sensorTask(void *param);
void actuatorTask(void *param);
void scheduleTask(void *param);
void networkTask(void *param);

// Helpers

// Apply a fully-loaded DeviceConfig to all subsystems and schedule callbacks.
// Called both at boot (from NVS) and when a fresh /init arrives over MQTT.
static void applyConfig(const DeviceConfig &config)
{
    automation->setTargets(config.targetTemp, config.targetHumidity);
    automation->enable();

    scheduler->getLightSchedule().setTiming(
        Timing(config.lightStartSec, config.lightDurationSec, config.lightIntervalSec));
    scheduler->getWaterSchedule().setTiming(
        Timing(config.waterStartSec, config.waterDurationSec, config.waterIntervalSec));

    display->setGreenType(config.greenType);
    display->setGrowth(config.growth);

    scheduler->getWaterSchedule().enable();
    scheduler->getLightSchedule().enable();
    if (config.blackout)
        scheduler->getLightSchedule().pause();
}

static void initializeHardware()
{
    Serial.println("Creating display...");
    display = new DisplayManager();
    display->begin();
    display->showBoot();

    Serial.println("Creating sensors...");
    sensors = new SensorManager();
    sensors->begin();

    Serial.println("Creating actuators...");
    actuators = new ActuatorManager();
    actuators->begin();

    Serial.println("Creating scheduler...");
    scheduler = new Scheduler();

    Serial.println("Creating automation...");
    automation = new AutomationController(*sensors, *actuators);

    Serial.println("Creating storage...");
    storage = new PersistenceManager();

    Serial.println("Creating WiFi manager...");
    wifiMgr = new EspWiFiManager();
}

static void setupWiFiButton()
{
    pinMode(PIN_WIFI_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_WIFI_BUTTON), handleResetButton, FALLING);
}

static void setupScheduleCallbacks()
{
    scheduler->getWaterSchedule().setCallbacks(
        [=]()
        { actuators->getPump().on(); },
        [=]()
        { actuators->getPump().off(); });

    scheduler->getLightSchedule().setCallbacks(
        [=]()
        { actuators->getLEDs().setColor(255, 255, 255); },
        [=]()
        { actuators->getLEDs().off(); });
}

static void loadAndApplyConfig()
{
    bool configured = storage->isConfigured();
    DeviceConfig config;
    if (configured)
    {
        Serial.println("Loading saved configuration...");
        config = storage->loadConfig();
        applyConfig(config);
    }
    else
    {
        config.growth = "seed";
        Serial.println("No saved configuration - awaiting MQTT setup");
    }
}

static void setupMQTT()
{
    Serial.println("Creating MQTT client...");
    mqttClient = &MQTTClient::getInstance(wifiMgr->getDeviceId());

    mqttClient->setActuators(actuators);
    mqttClient->setScheduler(scheduler);
    mqttClient->setAutomation(automation);
    mqttClient->setDisplay(display);

    MQTTCallbacks cbs;
    cbs.onConfig = [](const DeviceConfig &cfg)
    {
        storage->saveConfig(cfg);
        applyConfig(cfg);
    };
    cbs.onGrowth = [](const String &growth)
    {
        DeviceConfig cfg = storage->loadConfig();
        cfg.growth = growth;
        storage->saveConfig(cfg);
    };
    cbs.onBlackout = [](bool blackout)
    {
        DeviceConfig cfg = storage->loadConfig();
        cfg.blackout = blackout;
        storage->saveConfig(cfg);
    };
    cbs.onDelete = []()
    { storage->clear(); };

    mqttClient->setCallbacks(cbs);

    DeviceConfig config = storage->isConfigured() ? storage->loadConfig() : DeviceConfig();
    mqttClient->begin(storage->isConfigured(), config.greenType, config.growth);
}

static void initializeWiFi()
{
    Serial.println("Starting WiFi...");
    Serial.printf("Device ID: %s\n", wifiMgr->getDeviceId().c_str());
    if (!wifiMgr->begin())
    {
        Serial.println("Starting WiFi configuration portal...");
        display->showWiFiSetup(wifiMgr->getDeviceId());
        wifiMgr->startConfigPortal();
    }
}

static void startTasks()
{
    Serial.println("Creating tasks...");
    xTaskCreatePinnedToCore(sensorTask, "Sensor", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(actuatorTask, "Actuator", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(scheduleTask, "Schedule", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(networkTask, "Network", 8192, NULL, 2, NULL, 1);
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("\n\n=== MicroGrow Starting ===");

    initializeHardware();
    setupWiFiButton();
    setupScheduleCallbacks();
    loadAndApplyConfig();
    setupMQTT();
    initializeWiFi();

    Serial.println("Starting MQTT...");
    mqttClient->connect();

    actuators->getLEDs().flash(CRGB::Teal);
    startTasks();

    actuators->getLEDs().flash(CRGB::Green);
    Serial.println("\n=== MicroGrow Ready ===\n");
}

void loop()
{
    if (resetWiFiRequested)
    {
        resetWiFiRequested = false;
        Serial.println("WiFi reset requested");
        display->showWiFiSetup(wifiMgr->getDeviceId());
        wifiMgr->resetCredentials();
    }

    vTaskDelay(portMAX_DELAY);
}

// ============================================================================
// Tasks
// ============================================================================

void sensorTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        SensorReadings readings = sensors->read();

        if (wifiMgr->isConnected() && mqttClient->isInitialized() && readings.valid)
        {
            display->showSensorData(
                readings.temperature,
                readings.humidity,
                readings.waterLevelLow);
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

void actuatorTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        automation->update();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(ACTUATOR_UPDATE_INTERVAL_MS));
    }
}

void scheduleTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    int retryCount = 0;
    const int maxRetries = 60;

    while (!Scheduler::isTimeValid() && retryCount < maxRetries)
    {
        Serial.println("Waiting for NTP time sync...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        retryCount++;
    }

    if (!Scheduler::isTimeValid())
        Serial.println("WARNING: Scheduler starting without valid time");
    else
        Serial.println("Scheduler started with valid time");

    while (true)
    {
        scheduler->update();
        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(SCHEDULE_CHECK_INTERVAL_MS));
    }
}

void networkTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    TickType_t lastPublish = 0;
    time_t lastReading = 0;

    configTime(TIMEZONE_OFFSET_S, DST_OFFSET_S, NTP_SERVER_1, NTP_SERVER_2);

    while (true)
    {
        if (!wifiMgr->isConnected())
        {
            display->setWiFiStatus(false);
            Serial.println("WiFi disconnected, attempting reconnect...");
            actuators->getLEDs().flash(CRGB::Orange4);
            wifiMgr->reconnect();
        }
        else
        {
            display->setWiFiStatus(true);
        }

        if (wifiMgr->isConnected())
        {
            if (!mqttClient->isConnected())
            {
                display->setMQTTStatus(false);
                mqttClient->connect();
            }
            else
            {
                display->setMQTTStatus(true);
                mqttClient->loop();

                if (mqttClient->isInitialized())
                {
                    // Periodic sensor publish
                    TickType_t now = xTaskGetTickCount();
                    if (now - lastPublish > pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL_MS))
                    {
                        SensorReadings readings = sensors->read();
                        if (readings.valid)
                        {
                            mqttClient->publishSensorData(
                                readings.temperature,
                                readings.humidity,
                                readings.waterLevelLow,
                                actuators->getLEDs().getcurrentColor());
                            lastPublish = now;
                        }
                    }

                    // Periodic NVS reading save
                    time_t now_t;
                    time(&now_t);
                    if (now_t - lastReading >= READING_SAVE_INTERVAL_S)
                    {
                        SensorReadings readings = sensors->read();
                        if (readings.valid)
                        {
                            StoredReading r;
                            r.temperature = readings.temperature;
                            r.humidity = readings.humidity;
                            r.timestamp = now_t;
                            storage->saveReading(r);
                        }
                        lastReading = now_t;
                    }
                }
            }
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(20));
    }
}