#include "mqtt_client.h"
#include "../config/config.h"
#include "../control/automation.h"
#include "../control/scheduler.h"
#include "../hardware/actuators.h"
#include "../storage/persistence.h"

MQTTClient *MQTTClient::instance = nullptr;

MQTTClient &MQTTClient::getInstance(const String &deviceId)
{
  static MQTTClient singleton(deviceId);
  instance = &singleton;
  return singleton;
}

MQTTClient::MQTTClient(const String &deviceId)
    : client(espClient), deviceId(deviceId),
      initState(InitState::WAITING_FOR_ID), lastReconnectAttempt(0),
      lastPublish(0), actuators(nullptr), scheduler(nullptr),
      automation(nullptr)
{

  instance = this;
}

bool MQTTClient::begin()
{
  // Check if we have existing configuration
  PersistenceManager storage;
  if (storage.isConfigured())
  {
    Serial.println("Found existing configuration - marking as initialized");
    initState = InitState::COMPLETE;

    // Load the greenType for reference
    DeviceConfig config = storage.loadConfig();
    greenType = config.greenType;
  }
  else
  {
    Serial.println("No existing configuration - awaiting init messages");
    initState = InitState::WAITING_FOR_ID;
  }

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(staticCallback);
  client.setKeepAlive(MQTT_KEEPALIVE_S);
  client.setSocketTimeout(MQTT_SOCKET_TIMEOUT_S);

  Serial.printf("MQTT configured: %s:%d\n", MQTT_BROKER, MQTT_PORT);
  return true;
}

bool MQTTClient::connect()
{
  if (isConnected())
  {
    return true;
  }

  uint32_t now = millis();
  if (now - lastReconnectAttempt < MQTT_RECONNECT_DELAY_MS)
  {
    return false;
  }

  lastReconnectAttempt = now;

  String clientId = "esp32-" + deviceId;
  Serial.printf("MQTT connecting as %s...", clientId.c_str());

  if (client.connect(clientId.c_str()))
  {
    Serial.println(" connected");

    if (initState == InitState::COMPLETE)
    {
      // Already configured, subscribe to habitat topics only
      subscribeToHabitatTopics();
      Serial.println("Already configured - skipping init subscription");
    }
    else
    {
      // Awaiting configuration, subscribe to init topic
      subscribeToInitTopics();
      Serial.println("Awaiting configuration via init topic");
    }

    return true;
  }

  Serial.printf(" failed (rc=%d)\n", client.state());
  return false;
}

bool MQTTClient::isConnected() { return client.connected(); }

void MQTTClient::disconnect()
{
  if (isConnected())
  {
    client.disconnect();
    Serial.println("MQTT disconnected");
  }
}

void MQTTClient::loop()
{
  if (isConnected())
  {
    client.loop();
  }
}

void MQTTClient::subscribeToInitTopics()
{
  String initTopic = "microgrow/" + deviceId + "/init";
  client.subscribe(initTopic.c_str());
  Serial.printf("Subscribed to: %s\n", initTopic.c_str());
}

void MQTTClient::subscribeToHabitatTopics()
{
  String overrideTopic = "microgrow/" + deviceId + "/override";
  client.subscribe(overrideTopic.c_str());
  Serial.printf("Subscribed to: %s\n", overrideTopic.c_str());
}

bool MQTTClient::publishSensorData(float temp, float humidity, bool waterLow,
                                   bool lightOn)
{
  if (!isConnected())
  {
    return false;
  }

  String base = "microgrow/" + deviceId + "/";

  bool success = true;
  success &= client.publish((base + "temp").c_str(), String(temp, 1).c_str());
  success &=
      client.publish((base + "humidity").c_str(), String(humidity, 1).c_str());
  success &= client.publish((base + "water").c_str(),
                            String(waterLow ? 1 : 0).c_str());
  success &= client.publish((base + "light").c_str(),
                            String(lightOn ? 100 : 0).c_str());

  return success;
}

bool MQTTClient::publishStatus(const String &message)
{
  if (!isConnected())
  {
    return false;
  }

  String topic = "microgrow/" + deviceId + "/status";
  return client.publish(topic.c_str(), message.c_str());
}

void MQTTClient::staticCallback(char *topic, byte *payload,
                                unsigned int length)
{
  if (instance)
  {
    instance->messageCallback(topic, payload, length);
  }
}

void MQTTClient::messageCallback(char *topic, byte *payload,
                                 unsigned int length)
{
  // Convert payload to string
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++)
  {
    message += (char)payload[i];
  }

  Serial.printf("MQTT [%s]: %s\n", topic, message.c_str());

  // Parse JSON
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err)
  {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  String topicStr = String(topic);

  // Handle init messages
  if (topicStr.endsWith("/init"))
  {
    handleInit(doc);
    return;
  }

  // Handle override messages
  if (topicStr.endsWith("/override"))
  {
    handleOverride(doc);
    return;
  }
}

void MQTTClient::handleInit(JsonDocument &doc)
{
  // Check if this is a greenType message
  if (doc["greenType"].is<String>())
  {
    greenType = doc["greenType"].as<String>();

    float targetTemp = doc["target"]["temp"].as<float>();
    float targetHum = doc["target"]["humidity"].as<float>();

    Serial.printf("Received greenType: %s\n", greenType.c_str());

    // Set automation targets
    if (automation)
    {
      automation->setTargets(targetTemp, targetHum);
      automation->enable();
    }

    // Save to NVS
    PersistenceManager storage;
    storage.saveHabitatInfo(greenType);
    storage.saveTargets(targetTemp, targetHum);

    initState = InitState::WAITING_FOR_SCHEDULE;
  }
  // Check if this is schedule message
  else if (doc["light"].is<JsonObject>() && doc["water"].is<JsonObject>())
  {
    Serial.println("Received schedule configuration");

    // Extract schedule data
    uint32_t lightStart = doc["light"]["startTimeSec"].as<uint32_t>();
    uint32_t lightDur = doc["light"]["durationSec"].as<uint32_t>();
    uint32_t lightInt = doc["light"]["intervalSec"].as<uint32_t>();

    uint32_t waterStart = doc["water"]["startTimeSec"].as<uint32_t>();
    uint32_t waterDur = doc["water"]["durationSec"].as<uint32_t>();
    uint32_t waterInt = doc["water"]["intervalSec"].as<uint32_t>();

    Serial.printf("Light schedule: start=%u, duration=%u, interval=%u\n",
                  lightStart, lightDur, lightInt);
    Serial.printf("Water schedule: start=%u, duration=%u, interval=%u\n",
                  waterStart, waterDur, waterInt);

    // Configure schedules
    if (scheduler)
    {
      scheduler->getLightSchedule().setTiming(lightStart, lightDur, lightInt);
      scheduler->getLightSchedule().enable();

      scheduler->getWaterSchedule().setTiming(waterStart, waterDur, waterInt);
      scheduler->getWaterSchedule().enable();
    }

    // Save to NVS
    PersistenceManager storage;
    storage.saveLightSchedule(lightStart, lightDur, lightInt);
    storage.saveWaterSchedule(waterStart, waterDur, waterInt);

    initState = InitState::COMPLETE;

    // Configuration complete
    client.unsubscribe(("microgrow/" + deviceId + "/init").c_str());
    subscribeToHabitatTopics();

    Serial.println("Configuration complete!");
    saveConfiguration();
    publishStatus("configured");
  }
  else
  {
    Serial.println("ERROR: Unknown init message format");
    serializeJsonPretty(doc, Serial);
    Serial.println();
  }
}

void MQTTClient::handleOverride(JsonDocument &doc)
{
  if (!actuators)
  {
    Serial.println("Actuators not available");
    return;
  }

  int actuatorId = doc["actuator"].as<int>();
  bool enable = doc["enable"].as<bool>();

  Serial.printf("Override: actuator=%d, enable=%d\n", actuatorId, enable);

#define FAN_ID 0
#define WATER_PUMP_ID 1
#define LED_ID 2
#define MISTER_ID 3

  switch (actuatorId)
  {
  case FAN_ID:
  {
    Fan &fan = actuators->getFan();
    if (enable)
    {
      uint8_t value = doc["value"] | 255; // default full power if not provided
      fan.setManualOverride(true);
      if (value)
        fan.on();
      else
        fan.off();
    }
    else
    {
      fan.setManualOverride(false);
      // Automation will take over on next update()
    }
    break;
  }

  case WATER_PUMP_ID:
  {
    WaterPump &pump = actuators->getPump();
    if (enable)
    {
      if (scheduler)
        scheduler->getWaterSchedule().pause();

      pump.on();
      delay(2000); // Run for 2 seconds
      pump.off();

      if (scheduler)
        scheduler->getWaterSchedule().resume();
        }
    break;
  }

  case LED_ID:
  {
    LEDStrip &leds = actuators->getLEDs();
    if (enable)
    {
      uint8_t r = doc["r"].as<uint8_t>();
      uint8_t g = doc["g"].as<uint8_t>();
      uint8_t b = doc["b"].as<uint8_t>();
      leds.setColor(r, g, b);
      leds.setManualOverride(true);

      // Pause light schedule while manual override is active
      if (scheduler)
      {
        scheduler->getLightSchedule().pause();
      }
    }
    else
    {
      leds.off();
      leds.setManualOverride(false);

      // Resume light schedule
      if (scheduler)
      {
        scheduler->getLightSchedule().resume();
      }
    }
    break;
  }
  case MISTER_ID:
  {
    Mister &mister = actuators->getMister();
    if (enable)
    {
      uint8_t value = doc["value"] | 1; // non-zero means ON
      mister.setManualOverride(true);
      if (value)
        mister.on();
      else
        mister.off();
    }
    else
    {
      mister.setManualOverride(false);
      // Automation will handle it next cycle
    }
    break;
  }

  default:
    Serial.printf("Unknown actuator ID: %d\n", actuatorId);
    break;
  }
}

void MQTTClient::saveConfiguration()
{
  PersistenceManager storage;

  DeviceConfig config;
  config.greenType = greenType;

  if (automation)
  {
    config.targetTemp = automation->getTargets().temperature;
    config.targetHumidity = automation->getTargets().humidity;
  }

  // Get schedule timings from scheduler
  if (scheduler)
  {
    auto &light = scheduler->getLightSchedule().getTiming();
    auto &water = scheduler->getWaterSchedule().getTiming();

    config.lightStartSec = light.startSec;
    config.lightDurationSec = light.durationSec;
    config.lightIntervalSec = light.intervalSec;

    config.waterStartSec = water.startSec;
    config.waterDurationSec = water.durationSec;
    config.waterIntervalSec = water.intervalSec;
  }

  config.valid = true;
  storage.saveConfig(config);
}
