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

// ISR flags
volatile bool resetWiFiRequested = false;
volatile bool shutdownRequested = false;
volatile bool shutdownComplete = false;

volatile uint32_t lastResetPress = 0;
volatile uint32_t lastShutdownPress = 0;

#define DEBOUNCE_MS 200

void IRAM_ATTR handleResetButton()
{
    uint32_t now = millis();
    if (now - lastResetPress > DEBOUNCE_MS)
    {
        lastResetPress = now;
        resetWiFiRequested = true;
    }
}

void IRAM_ATTR handlePowerButton()
{
    uint32_t now = millis();
    if (now - lastShutdownPress > DEBOUNCE_MS)
    {
        lastShutdownPress = now;
        shutdownRequested = true;
    }
}

// FreeRTOS Tasks
void sensorTask(void *param);
void actuatorTask(void *param);
void scheduleTask(void *param);
void networkTask(void *param);

// ============================================================================
// Helpers
// ============================================================================

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

static void performShutdown()
{
    Serial.println("\n=== Shutdown requested ===");

    // 1. Stop automation and schedules - prevents core-1 tasks from issuing
    //    further actuator commands or NVS writes while we work.
    if (automation)
        automation->disable();

    if (scheduler)
    {
        scheduler->getLightSchedule().disable();
        scheduler->getWaterSchedule().disable();
    }

    // 2. Signal tasks to stop, then give core 1 one tick to exit any open
    //    prefs.begin()/putBytes()/end() sequence it may be mid-flight in.
    shutdownComplete = true;
    vTaskDelay(pdMS_TO_TICKS(10));

    // 3. Flush NVS - commit a final sensor reading so data from the current
    //    interval is not lost if the user unplugs before the next periodic save.
    if (sensors && storage)
    {
        SensorReadings r = sensors->read();
        if (r.valid)
        {
            time_t now_t;
            time(&now_t);
            storage->flushPendingReading(r.temperature, r.humidity, now_t);
        }
    }

    // 4. All actuators off - pump, fan, mister, LEDs
    if (actuators)
        actuators->allOff();

    // 5. Clean network disconnect
    if (mqttClient && mqttClient->isConnected())
        mqttClient->disconnect();
    if (wifiMgr && wifiMgr->isConnected())
        WiFi.disconnect(true);

    // 6. Shutdown screen + solid red LEDs
    if (display)
        display->showShutdown();
    if (actuators)
        actuators->getLEDs().setColor(CRGB::Red);

    Serial.println("=== Safe to unplug ===\n");
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(1000);
    Serial.println("\n\n=== MicroGrow Starting ===");

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

    // Setup buttons
    pinMode(PIN_WIFI_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_WIFI_BUTTON), handleResetButton, FALLING);
    pinMode(PIN_POWER_BUTTON, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(PIN_POWER_BUTTON), handlePowerButton, FALLING);

    Serial.println("8. Creating MQTT client...");
    mqttClient = &MQTTClient::getInstance(wifiMgr->getDeviceId());
    yield();

    // Wire MQTT dependencies
    mqttClient->setActuators(actuators);
    mqttClient->setScheduler(scheduler);
    mqttClient->setAutomation(automation);
    mqttClient->setDisplay(display);

    MQTTCallbacks cbs;
    cbs.onConfig = [](const DeviceConfig &cfg)
    {
        actuators->getLEDs().flash(CRGB::Green);
        storage->saveConfig(cfg);
        display->setGreenType(cfg.greenType);
        display->setGrowth(cfg.growth);
        applyConfig(cfg);
    };
    cbs.onGrowth = [](const String &growth)
    {
        if (growth == "harvest")
        {
            scheduler->getLightSchedule().disable();
            scheduler->getWaterSchedule().disable();
            actuators->getLEDs().setColor(CRGB::Purple);
        }
        DeviceConfig cfg = storage->loadConfig();
        cfg.growth = growth;
        display->setGrowth(growth);
        storage->saveConfig(cfg);
    };
    cbs.onBlackout = [](bool blackout)
    {
        DeviceConfig cfg = storage->loadConfig();
        cfg.blackout = blackout;
        storage->saveConfig(cfg);
    };
    cbs.onDelete = []()
    {
        storage->clear();
    };

    mqttClient->setCallbacks(cbs);

    bool isConfigured = storage->isConfigured();
    // Load config and prime MQTT with current state before WiFi comes up
    DeviceConfig config = isConfigured ? storage->loadConfig() : DeviceConfig();
    if (isConfigured)
        applyConfig(config);
    else
        config.growth = "seedling";

    mqttClient->begin(isConfigured, config.greenType, config.growth);

    scheduler->getWaterSchedule().setCallbacks(
        [=]()
        {
            mqttClient->publishPulse("watering_on");
            actuators->getPump1().on();
            actuators->getPump2().on();
        },
        [=]()
        {
            actuators->getPump1().off();
            actuators->getPump2().off();
        });

    scheduler->getLightSchedule().setCallbacks(
        [=]()
        { actuators->getLEDs().setColor(255, 255, 255); },
        [=]()
        { actuators->getLEDs().off(); });

    // Initialize WiFi
    Serial.println("9. Starting WiFi...");
    Serial.printf("Device ID: %s\n", wifiMgr->getDeviceId().c_str());

    if (!wifiMgr->begin())
    {
        // No saved WiFi credentials - start config portal
        WiFiSetupReason reason = isConfigured ? WiFiSetupReason::PORTAL_WITH_CONFIG : WiFiSetupReason::NO_WIFI_NO_CONFIG;
        display->showWiFiSetup(wifiMgr->getDeviceId(), reason);
        resetWiFiRequested = true;
        wifiMgr->runConfigPortal([]()
                                 { return (bool)shutdownRequested; });
        resetWiFiRequested = false;

        if (shutdownRequested)
        {
            shutdownRequested = false;
            performShutdown();
            while (true)
                vTaskDelay(portMAX_DELAY);
        }
        actuators->getLEDs().flash(CRGB::Yellow);
    }
    else if (!isConfigured)
    {
        // WiFi connected but no DeviceConfig yet - MQTT will deliver it
        display->showWiFiSetup(wifiMgr->getDeviceId(), WiFiSetupReason::WIFI_NO_CONFIG);
    }
    else
    {
        // Fully configured device reconnecting after reset/power cycle
        display->showWiFiSetup(wifiMgr->getDeviceId(), WiFiSetupReason::CONFIG_NO_WIFI);
    }
    yield();

    // Initialize MQTT
    Serial.println("10. Starting MQTT...");
    mqttClient->begin(isConfigured, config.greenType, config.growth);
    yield();

    // Start FreeRTOS tasks
    Serial.println("11. Creating FreeRTOS tasks...");
    xTaskCreatePinnedToCore(sensorTask, "Sensor", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(actuatorTask, "Actuator", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(scheduleTask, "Schedule", 8192, NULL, 1, NULL, 1);
    xTaskCreatePinnedToCore(networkTask, "Network", 8192, NULL, 2, NULL, 1);

    Serial.println("\n=== MicroGrow Ready ===\n");
}

// ============================================================================
// Loop
// ============================================================================

void loop()
{
    if (shutdownRequested)
    {
        shutdownRequested = false;
        performShutdown();
        // Block loop() permanently - device stays alive to show the screen
        while (true)
            vTaskDelay(portMAX_DELAY);
    }

    if (resetWiFiRequested)
    {
        Serial.println("WiFi reset requested");
        wifiMgr->disconnect();
        wifiMgr->resetCredentials();

        WiFiSetupReason reason = storage->isConfigured() ? WiFiSetupReason::PORTAL_WITH_CONFIG : WiFiSetupReason::NO_WIFI_NO_CONFIG;
        display->showWiFiSetup(wifiMgr->getDeviceId(), reason);
        display->setWiFiStatus(false);
        display->setMQTTStatus(false);

        wifiMgr->runConfigPortal([]()
                                 { return (bool)shutdownRequested; });
        resetWiFiRequested = false;

        if (shutdownRequested)
        {
            shutdownRequested = false;
            performShutdown();
            while (true)
                vTaskDelay(portMAX_DELAY);
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}

// ============================================================================
// FreeRTOS Tasks
// ============================================================================

void sensorTask(void *param)
{
    TickType_t lastWakeTime = xTaskGetTickCount();

    while (true)
    {
        if (shutdownComplete)
            vTaskSuspend(nullptr);

        if (wifiMgr->isConnected() && mqttClient->isInitialized())
        {
            SensorReadings readings = sensors->read();

            if (display /*&& readings.valid*/)
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
        if (shutdownComplete)
            vTaskSuspend(nullptr);

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
        if (shutdownComplete)
            vTaskSuspend(nullptr);
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
        if (shutdownComplete)
            vTaskSuspend(nullptr);

        if (wifiMgr->isConnected() && mqttClient->isInitialized())
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

        vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(100));
    }
}