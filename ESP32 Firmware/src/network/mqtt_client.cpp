#include "mqtt_client.h"
#include "../config/config.h"
#include "../control/automation.h"
#include "../control/scheduler.h"
#include "../hardware/actuators.h"

MQTTClient *MQTTClient::instance = nullptr;

MQTTClient &MQTTClient::getInstance(const String &deviceId)
{
  static MQTTClient singleton(deviceId);
  instance = &singleton;
  return singleton;
}

MQTTClient::MQTTClient(const String &deviceId)
    : client(espClient), deviceId(deviceId),
      initState(InitState::UNCONFIGURED), lastReconnectAttempt(0),
      lastPublish(0), actuators(nullptr), scheduler(nullptr),
      automation(nullptr), display(nullptr)
{
  instance = this;
}

bool MQTTClient::begin(bool alreadyConfigured, const String &gt, const String &gr)
{
  greenType = gt;
  growth = gr;

  if (alreadyConfigured)
  {
    initState = InitState::COMPLETE;
    Serial.println("MQTT: device already configured");
  }
  else
  {
    initState = InitState::UNCONFIGURED;
    Serial.println("MQTT: awaiting /init message");
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
    return true;

  uint32_t now = millis();
  if (now - lastReconnectAttempt < MQTT_RECONNECT_DELAY_MS)
    return false;

  lastReconnectAttempt = now;

  String clientId = "esp32-" + deviceId;
  Serial.printf("MQTT connecting as %s...", clientId.c_str());

  if (client.connect(clientId.c_str()))
  {
    Serial.println(" connected");

    if (initState == InitState::COMPLETE)
    {
      subscribeToHabitatTopics();
      Serial.println("Already configured - subscribed to habitat topics");
    }
    else
    {
      subscribeToInitTopics();
      Serial.println("Awaiting configuration via /init");
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
    client.loop();
}

void MQTTClient::subscribeToInitTopics()
{
  String topic = "microgrow/" + deviceId + "/init";
  client.subscribe(topic.c_str());
  Serial.printf("Subscribed to: %s\n", topic.c_str());
}

void MQTTClient::subscribeToHabitatTopics()
{
  String base = "microgrow/" + deviceId + "/";
  client.subscribe((base + "override").c_str());
  client.subscribe((base + "refresh").c_str());
  client.subscribe((base + "growth").c_str());
  client.subscribe((base + "delete").c_str());
  client.subscribe((base + "blackout").c_str());
  Serial.printf("Subscribed to habitat topics for %s\n", deviceId.c_str());
}

// Publishing

bool MQTTClient::publishSensorData(float temp, float humidity, bool waterLow, CRGB color)
{
  if (!isConnected())
    return false;

  String base = "microgrow/" + deviceId + "/";

  JsonDocument colorDoc;
  colorDoc["r"] = color.r;
  colorDoc["g"] = color.g;
  colorDoc["b"] = color.b;
  String colorJson;
  serializeJson(colorDoc, colorJson);

  bool ok = true;
  ok &= client.publish((base + "temp").c_str(), String(temp, 1).c_str());
  ok &= client.publish((base + "humidity").c_str(), String(humidity, 1).c_str());
  ok &= client.publish((base + "water").c_str(), String(waterLow ? 1 : 0).c_str());
  ok &= client.publish((base + "light").c_str(), colorJson.c_str());
  return ok;
}

bool MQTTClient::publishStatus(const String &message)
{
  if (!isConnected())
    return false;

  String topic = "microgrow/" + deviceId + "/status";
  return client.publish(topic.c_str(), message.c_str());
}

// Callback dispatch

void MQTTClient::staticCallback(char *topic, byte *payload, unsigned int length)
{
  if (instance)
    instance->messageCallback(topic, payload, length);
}

void MQTTClient::messageCallback(char *topic, byte *payload, unsigned int length)
{
  String message;
  message.reserve(length);
  for (unsigned int i = 0; i < length; i++)
    message += (char)payload[i];

  Serial.printf("MQTT [%s]: %s\n", topic, message.c_str());

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, message);
  if (err)
  {
    Serial.printf("JSON parse error: %s\n", err.c_str());
    return;
  }

  String t = String(topic);

  if (t.endsWith("/init"))
  {
    handleInit(doc);
    return;
  }
  if (t.endsWith("/override"))
  {
    handleOverride(doc);
    return;
  }
  if (t.endsWith("/refresh"))
  {
    handleRefresh(doc);
    return;
  }
  if (t.endsWith("/growth"))
  {
    handleGrowth(doc);
    return;
  }
  if (t.endsWith("/delete"))
  {
    handleDelete();
    return;
  }
  if (t.endsWith("/blackout"))
  {
    handleBlackout(doc);
    return;
  }
}

// Message handlers

/*
 * /init - single message, complete configuration.
 *
 * Expected JSON example:
 * {
 *   "greenType":       String,
 *   "targetTemp":      float,
 *   "targetHumidity":  float,
 *   "blackout":        bool,
 *   "light": { "startSec": uint32_t, "durationSec": uint32_t, "intervalSec": uint32_t },
 *   "water": { "startSec": uint32_t, "durationSec": uint32_t, "intervalSec": uint32_t }
 * }
 */
void MQTTClient::handleInit(JsonDocument &doc)
{
  if (!doc["greenType"].is<String>() ||
      !doc["light"].is<JsonObject>() ||
      !doc["water"].is<JsonObject>())
  {
    Serial.println("ERROR: /init message missing required fields");
    serializeJsonPretty(doc, Serial);
    Serial.println();
    return;
  }

  // Build a complete DeviceConfig from the single message.
  DeviceConfig config;
  config.greenType = doc["greenType"].as<String>();
  config.growth = growth; // preserve current growth stage
  config.targetTemp = doc["targetTemp"].as<float>();
  config.targetHumidity = doc["targetHumidity"].as<float>();
  config.blackout = doc["blackout"] | false;

  config.lightStartSec = doc["light"]["startSec"].as<uint32_t>();
  config.lightDurationSec = doc["light"]["durationSec"].as<uint32_t>();
  config.lightIntervalSec = doc["light"]["intervalSec"].as<uint32_t>();

  config.waterStartSec = doc["water"]["startSec"].as<uint32_t>();
  config.waterDurationSec = doc["water"]["durationSec"].as<uint32_t>();
  config.waterIntervalSec = doc["water"]["intervalSec"].as<uint32_t>();

  Serial.printf("Init: greenType=%s, temp=%.1f, hum=%.1f, blackout=%d\n",
                config.greenType.c_str(), config.targetTemp,
                config.targetHumidity, config.blackout);
  Serial.printf("  Light: start=%u dur=%u int=%u\n",
                config.lightStartSec, config.lightDurationSec, config.lightIntervalSec);
  Serial.printf("  Water: start=%u dur=%u int=%u\n",
                config.waterStartSec, config.waterDurationSec, config.waterIntervalSec);

  // Apply to in-memory subsystems.
  greenType = config.greenType;
  blackout = config.blackout;

  if (automation)
  {
    automation->setTargets(config.targetTemp, config.targetHumidity);
    automation->enable();
  }

  if (scheduler)
  {
    scheduler->getLightSchedule().setTiming(
        config.lightStartSec, config.lightDurationSec, config.lightIntervalSec);
    scheduler->getLightSchedule().enable();
    if (blackout)
      scheduler->getLightSchedule().pause();

    scheduler->getWaterSchedule().setTiming(
        config.waterStartSec, config.waterDurationSec, config.waterIntervalSec);
    scheduler->getWaterSchedule().enable();
  }

  if (display)
    display->setGreenType(config.greenType);

  // Delegate persistence to main.cpp via callback - MQTTClient does not
  // touch PersistenceManager directly.
  if (callbacks.onConfig)
    callbacks.onConfig(config);

  // Transition to configured state and swap subscriptions.
  initState = InitState::COMPLETE;
  client.unsubscribe(("microgrow/" + deviceId + "/init").c_str());
  subscribeToHabitatTopics();

  publishStatus("configured");
  Serial.println("Init complete");
}

// Pump pulse task (file-local, cancellable)
struct PumpPulseContext
{
  WaterPump *pump;
  Scheduler *scheduler;
  uint32_t durationMs;
};

static TaskHandle_t pumpPulseHandle = NULL;
static volatile bool pumpPulseActive = false;

static void pumpPulseTask(void *param)
{
  PumpPulseContext *ctx = static_cast<PumpPulseContext *>(param);

  WaterPump *pump = ctx->pump;
  Scheduler *scheduler = ctx->scheduler;
  uint32_t durationMs = ctx->durationMs;

  pump->on();

  // Sleep for duration (will be interrupted if task is deleted)
  vTaskDelay(pdMS_TO_TICKS(durationMs));

  // Normal completion path
  pump->off();
  pump->setManualOverride(false);

  if (scheduler)
    scheduler->getWaterSchedule().resume();

  pumpPulseActive = false;
  pumpPulseHandle = NULL;

  delete ctx;
  vTaskDelete(NULL);
}

/*
 * /override - manual actuator control.
 *
 * Expected JSON example:
 * { "actuator": <ActuatorId>, "enable": <bool>, ... }
 *
 * Fan:   { "actuator": 0, "enable": true }
 * Pump:  { "actuator": 1, "enable": true }
 * LED:   { "actuator": 2, "enable": true, "color": { "r": 255, "g": 255, "b": 255 } }
 * Mister:{ "actuator": 3, "enable": true }
 */
void MQTTClient::handleOverride(JsonDocument &doc)
{
  if (!actuators)
  {
    Serial.println("Override: actuators not available");
    return;
  }

  ActuatorId id = static_cast<ActuatorId>(doc["actuator"].as<int>());
  bool en = doc["enable"].as<bool>();

  Serial.printf("Override: actuator=%d, enable=%d\n", (int)id, en);

  switch (id)
  {
  case ActuatorId::FAN:
  {
    Fan &fan = actuators->getFan();
    fan.setManualOverride(en);
    if (en)
      fan.on();
    else
      fan.off();
    break;
  }

  case ActuatorId::WATER_PUMP:
  {
    WaterPump &pump = actuators->getPump();

    if (en)
    {
      if (pumpPulseActive)
      {
        Serial.println("Pump pulse already active, ignoring");
        break;
      }

      pump.setManualOverride(true);

      uint32_t durationMs = 0;
      if (scheduler)
      {
        durationMs =
            scheduler->getWaterSchedule().getTiming().durationSec * 1000;
        scheduler->getWaterSchedule().pause();
      }

      PumpPulseContext *ctx = new PumpPulseContext{
          &pump,
          scheduler,
          durationMs};

      BaseType_t ok = xTaskCreatePinnedToCore(
          pumpPulseTask,
          "PumpPulse",
          4096,
          ctx,
          1,
          &pumpPulseHandle,
          1);

      if (ok == pdPASS)
      {
        pumpPulseActive = true;
      }
      else
      {
        Serial.println("Failed to create pump pulse task");
        delete ctx;
        pump.setManualOverride(false);
      }
    }
    else
    {
      // Cancel running pulse if active
      if (pumpPulseHandle != NULL)
      {
        vTaskDelete(pumpPulseHandle);
        pumpPulseHandle = NULL;
      }

      pump.off();
      pump.setManualOverride(false);

      if (scheduler)
        scheduler->getWaterSchedule().resume();

      pumpPulseActive = false;
    }
    break;
  }

  case ActuatorId::LED:
  {
    LEDStrip &leds = actuators->getLEDs();
    if (en)
    {
      uint8_t r = doc["color"]["r"].as<uint8_t>();
      uint8_t g = doc["color"]["g"].as<uint8_t>();
      uint8_t b = doc["color"]["b"].as<uint8_t>();
      leds.setColor(r, g, b);
      leds.setManualOverride(true);
      if (scheduler)
        scheduler->getLightSchedule().pause();
    }
    else
    {
      leds.off();
      leds.setManualOverride(false);
      if (scheduler)
        scheduler->getLightSchedule().resume();
    }
    break;
  }

  case ActuatorId::MISTER:
  {
    Mister &mister = actuators->getMister();
    mister.setManualOverride(en);
    if (en)
      mister.on();
    else
      mister.off();
    break;
  }

  default:
    Serial.printf("Override: unknown actuator id %d\n", (int)id);
    break;
  }
}

/*
 * /refresh - publish stored readings back to the broker, then clear the buffer.
 *
 * Expected JSON example: {} (any message on this topic triggers the dump)
 *
 * Readings are published to microgrow/<id>/history (not /refresh, to avoid
 * the device receiving its own published messages).
 */
void MQTTClient::handleRefresh(JsonDocument &doc)
{
  PersistenceManager storage;
  uint8_t count = storage.getReadingCount();
  if (count == 0)
  {
    Serial.println("Refresh: no readings stored");
    return;
  }

  StoredReading *readings = new StoredReading[count];
  storage.getAllReadings(readings);

  String outTopic = "microgrow/" + deviceId + "/history";
  Serial.printf("Refresh: publishing %u readings to %s\n", count, outTopic.c_str());

  char timeStr[20];
  for (uint8_t i = 0; i < count; i++)
  {
    JsonDocument msg;
    msg["temp"] = readings[i].temperature;
    msg["humidity"] = readings[i].humidity;
    struct tm *ti = localtime(&readings[i].timestamp);
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", ti);
    msg["timestamp"] = timeStr;

    String json;
    serializeJson(msg, json);
    client.publish(outTopic.c_str(), json.c_str());
  }

  delete[] readings;
  storage.clearReadings();
}

/*
 * /growth - update the plant growth stage.
 *
 * Expected JSON example: { "growthStage": <1|2> }
 *  1 = sapling, 2 = grown
 */
void MQTTClient::handleGrowth(JsonDocument &doc)
{
  int32_t g = doc["growthStage"].as<int32_t>();

  if (g == 1)
    growth = "sapling";
  else if (g == 2)
    growth = "grown";

  Serial.printf("Growth: stage %d -> %s\n", g, growth.c_str());

  if (display)
  {
    display->clearImg();
    display->setGrowth(growth);
  }

  if (callbacks.onGrowth)
    callbacks.onGrowth(growth);
}

/*
 * /delete - wipe configuration and return to unconfigured state.
 *
 * Expected JSON example: {} (any message triggers the delete)
 */
void MQTTClient::handleDelete()
{
  Serial.println("Delete: clearing configuration");

  if (scheduler)
  {
    scheduler->getLightSchedule().disable();
    scheduler->getWaterSchedule().disable();
  }

  if (callbacks.onDelete)
    callbacks.onDelete();

  initState = InitState::UNCONFIGURED;
  client.unsubscribe(("microgrow/" + deviceId + "/override").c_str());
  client.unsubscribe(("microgrow/" + deviceId + "/refresh").c_str());
  client.unsubscribe(("microgrow/" + deviceId + "/growth").c_str());
  client.unsubscribe(("microgrow/" + deviceId + "/delete").c_str());
  client.unsubscribe(("microgrow/" + deviceId + "/blackout").c_str());
  subscribeToInitTopics();
}

/*
 * /blackout - enable or disable the light blackout mode.
 *
 * Expected JSON example: { "blackout": <bool> }
 */
void MQTTClient::handleBlackout(JsonDocument &doc)
{
  if (!doc["blackout"].is<bool>() && !doc["blackout"].is<int>())
  {
    Serial.println("Blackout: missing 'blackout' field");
    return;
  }

  blackout = doc["blackout"].as<bool>();
  Serial.printf("Blackout: %s\n", blackout ? "ON" : "OFF");

  if (scheduler)
  {
    if (blackout)
      scheduler->getLightSchedule().pause();
    else
      scheduler->getLightSchedule().resume();
  }

  if (callbacks.onBlackout)
    callbacks.onBlackout(blackout);
}