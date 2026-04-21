#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <functional>
#include <FastLED.h>
#include "../hardware/display.h"
#include "../storage/persistence.h"

// Forward declarations
class ActuatorManager;
class Scheduler;
class AutomationController;

// Two states: either we have a full config or we don't.
enum class InitState
{
    UNCONFIGURED,
    COMPLETE
};

// Actuator IDs used in /override messages.
enum class ActuatorId : int
{
    FAN = 0,
    WATER_PUMP = 1,
    LED = 2,
    MISTER = 3
};

// Callbacks to apply configuration changes to other subsystems.
// MQTTClient does not directly interact with PersistenceManager or DisplayManager,
// so these callbacks allow MQTTClient to delegate those responsibilities to main
struct MQTTCallbacks
{
    // Full config received via /init - persist and apply everything.
    std::function<void(const DeviceConfig &)> onConfig;

    // Growth stage changed via /growth.
    std::function<void(const String &growth)> onGrowth;

    // Blackout flag changed via /blackout.
    std::function<void(bool blackout)> onBlackout;

    // Device reset requested via /delete.
    std::function<void()> onDelete;
};

class MQTTClient
{
public:
    static MQTTClient &getInstance(const String &deviceId);

    // Call after wiring up all dependencies. Pass current config state so the
    // client knows whether to subscribe to /init or habitat topics.
    bool begin(bool alreadyConfigured, const String &greenType, const String &growth);

    // Connection management
    bool connect();
    bool isConnected();
    void disconnect();
    void loop();

    // Publishing
    bool publishSensorData(float temp, float humidity, bool waterLow, CRGB color);
    bool publishStatus(const String &message);

    // Configuration state
    bool isInitialized() const { return initState == InitState::COMPLETE; }

    // External dependencies - set before calling begin()
    void setActuators(ActuatorManager *act) { actuators = act; }
    void setScheduler(Scheduler *sched) { scheduler = sched; }
    void setAutomation(AutomationController *auto_) { automation = auto_; }
    void setDisplay(DisplayManager *disp) { display = disp; }
    void setCallbacks(const MQTTCallbacks &cbs) { callbacks = cbs; }

private:
    MQTTClient(const String &deviceId);
    MQTTClient(const MQTTClient &) = delete;
    MQTTClient &operator=(const MQTTClient &) = delete;

    WiFiClient espClient;
    PubSubClient client;
    String deviceId;
    String greenType;
    String growth;
    bool blackout = false;

    InitState initState;
    uint32_t lastReconnectAttempt;
    uint32_t lastPublish;

    // External dependencies
    ActuatorManager *actuators;
    Scheduler *scheduler;
    AutomationController *automation;
    DisplayManager *display;
    MQTTCallbacks callbacks;

    // Message handlers
    void handleInit(JsonDocument &doc);
    void handleOverride(JsonDocument &doc);
    void handleRefresh(JsonDocument &doc);
    void handleGrowth(JsonDocument &doc);
    void handleDelete();
    void handleBlackout(JsonDocument &doc);
    void messageCallback(char *topic, byte *payload, unsigned int length);

    // Subscription management
    void subscribeToInitTopics();
    void subscribeToHabitatTopics();

    // Static callback wrapper for PubSubClient
    static void staticCallback(char *topic, byte *payload, unsigned int length);
    static MQTTClient *instance;
};