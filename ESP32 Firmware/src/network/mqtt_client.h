#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include "../hardware/display.h"

// Forward declarations
class SensorManager;
class ActuatorManager;
class Scheduler;
class AutomationController;

enum class InitState
{
    WAITING_FOR_ID,
    WAITING_FOR_SCHEDULE,
    COMPLETE
};

class MQTTClient
{
public:
    static MQTTClient &getInstance(const String &deviceId);

    // Initialization
    bool begin();

    // Connection management
    bool connect();
    bool isConnected();
    void disconnect();
    void loop(); // Must be called regularly

    // Publishing
    bool publishSensorData(float temp, float humidity, bool waterLow, bool lightOn);
    bool publishStatus(const String &message);

    // Configuration state
    bool isInitialized() const { return initState == InitState::COMPLETE; }
    const String &getGreenType() const { return greenType; }

    // Set external dependencies
    void setActuators(ActuatorManager *act) { actuators = act; }
    void setScheduler(Scheduler *sched) { scheduler = sched; }
    void setAutomation(AutomationController *auto_) { automation = auto_; }
    void setDisplay(DisplayManager *disp_) { display = disp_; }

private:
    MQTTClient(const String &deviceId);
    MQTTClient(const MQTTClient &) = delete;
    MQTTClient &operator=(const MQTTClient &) = delete;

    WiFiClient espClient;
    PubSubClient client;
    String deviceId;
    String greenType;
    String growth;

    InitState initState;
    uint32_t lastReconnectAttempt;
    uint32_t lastPublish;

    // External dependencies
    ActuatorManager *actuators;
    Scheduler *scheduler;
    AutomationController *automation;
    DisplayManager *display;

    // Message handlers
    void handleInit(JsonDocument &doc);
    void handleOverride(JsonDocument &doc);
    void handleRefresh(JsonDocument &doc);
    void handleGrowth(JsonDocument &doc);
    void messageCallback(char *topic, byte *payload, unsigned int length);

    // Subscription management
    void subscribeToInitTopics();
    void subscribeToHabitatTopics();

    // Save configuration
    void saveConfiguration();
    void saveHabitat();

    // Static callback wrapper
    static void staticCallback(char *topic, byte *payload, unsigned int length);
    static MQTTClient *instance;
};
