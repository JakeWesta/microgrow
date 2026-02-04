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

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("\n\n=== MicroGrow Starting ===");

    // Initialize objects in controlled order
    Serial.println("1. Creating display...");
    display = new DisplayManager();
    display->begin();
    display->showBoot();
    yield();

    Serial.println("2. Creating sensors...");
    sensors = new SensorManager();
    sensors->begin();
    yield();

    Serial.println("3. Creating actuators...");
    actuators = new ActuatorManager();
    actuators->begin();
    yield();

    Serial.println("4. Creating scheduler...");
    scheduler = new Scheduler();
    yield();

    Serial.println("5. Creating automation...");
    automation = new AutomationController(*sensors, *actuators);
    yield();

    Serial.println("6. Creating storage...");
    storage = new PersistenceManager();
    yield();

    Serial.println("7. Creating WiFi manager...");
    wifiMgr = new EspWiFiManager();
    yield();

    // Setup WiFi reset button
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), handleResetButton, FALLING);

    Serial.println("8. Creating MQTT client...");
    mqttClient = &MQTTClient::getInstance(wifiMgr->getDeviceId());
    yield();

    // Wire MQTT dependencies
    mqttClient->setActuators(actuators);
    mqttClient->setScheduler(scheduler);
    mqttClient->setAutomation(automation);

    if (storage->isConfigured())
    {
        Serial.println("Loading saved configuration...");

        DeviceConfig config = storage->loadConfig();

        automation->setTargets(config.targetTemp, config.targetHumidity);
        automation->enable();

        scheduler->getLightSchedule().setTiming(
            Timing(config.lightStartSec,
                   config.lightDurationSec,
                   config.lightIntervalSec));
        scheduler->getLightSchedule().setCallbacks(
            [=]()
            { actuators->getLEDs().setColor(255, 255, 255); },
            [=]()
            { actuators->getLEDs().off(); });
        scheduler->getLightSchedule().enable();

        scheduler->getWaterSchedule().setTiming(
            Timing(config.waterStartSec,
                   config.waterDurationSec,
                   config.waterIntervalSec));
        scheduler->getWaterSchedule().setCallbacks(
            [=]()
            { actuators->getPump().on(); },
            [=]()
            { actuators->getPump().off(); });
        scheduler->getWaterSchedule().enable();

        display->showDeviceInfo(wifiMgr->getDeviceId(), config.greenType);
    }
    else
    {
        Serial.println("No saved configuration - awaiting setup");
    }

    // Initialize WiFi
    Serial.println("9. Starting WiFi...");
    Serial.printf("Device ID: %s\n", wifiMgr->getDeviceId().c_str());
    if (!wifiMgr->begin())
    {
        Serial.println("Starting WiFi configuration portal...");
        display->showWiFiSetup(wifiMgr->getDeviceId());
        wifiMgr->startConfigPortal();
    }
    yield();

    // Initialize MQTT
    Serial.println("10. Starting MQTT...");
    mqttClient->begin();
    yield();

    // Start FreeRTOS tasks
    Serial.println("11. Creating FreeRTOS tasks...");
    xTaskCreatePinnedToCore(sensorTask, "Sensor", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(actuatorTask, "Actuator", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(scheduleTask, "Schedule", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(networkTask, "Network", 8192, NULL, 2, NULL, 1);

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
// FreeRTOS Tasks
// ============================================================================

void sensorTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        SensorReadings readings = sensors->read();

        if (readings.valid)
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
        actuators->updateAll();

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(ACTUATOR_UPDATE_INTERVAL_MS));
    }
}

void scheduleTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();
    int retryCount = 0;
    const int maxRetries = 60; // Wait up to 60 seconds for time sync

    while (!Scheduler::isTimeValid() && retryCount < maxRetries)
    {
        Serial.println("Waiting for NTP time sync...");
        vTaskDelay(pdMS_TO_TICKS(1000));
        retryCount++;
    }

    if (!Scheduler::isTimeValid())
    {
        Serial.println("WARNING: Scheduler starting without valid time");
    }
    else
    {
        Serial.println("Scheduler started with valid time");
    }

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

    configTime(TIMEZONE_OFFSET_S, DST_OFFSET_S, NTP_SERVER_1, NTP_SERVER_2);

    while (true)
    {
        if (!wifiMgr->isConnected())
        {
            Serial.println("WiFi disconnected, attempting reconnect...");
            display->setWiFiStatus(false);
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
                    TickType_t now = xTaskGetTickCount();
                    if (now - lastPublish > pdMS_TO_TICKS(MQTT_PUBLISH_INTERVAL_MS))
                    {
                        SensorReadings readings = sensors->getLastReadings();
                        if (readings.valid)
                        {
                            mqttClient->publishSensorData(
                                readings.temperature,
                                readings.humidity,
                                readings.waterLevelLow,
                                actuators->getLEDs().isOn());
                            lastPublish = now;
                        }
                    }
                }
            }
        }

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));
    }
}