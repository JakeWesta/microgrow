#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <FastLED.h>
#include "defs.h"

// Configuration
const char* MQTT_BROKER = "10.2.84.7";
const int MQTT_PORT = 1883;
const uint32_t WIFI_TIMEOUT_MS = 10000;
const uint32_t MQTT_TIMEOUT_MS = 10000;
const uint32_t RECONNECT_DELAY_MS = 5000;
const uint32_t PUBLISH_INTERVAL_MS = 5000;

WiFiClient espClient;
PubSubClient client(espClient);

extern Shared shared;
extern SemaphoreHandle_t mutex;
extern volatile bool portalRequested;
extern bool initialized;
extern CRGB leds[NUM_LEDS];
extern Preferences prefs;
String habitatId = "";
String green = "";
bool hasId = false;
bool hasSchedule = false;

void saveConfigToNVS() {
    prefs.begin("microgrow", false);

    xSemaphoreTake(mutex, portMAX_DELAY);
    prefs.putBool("init", true);
    prefs.putString("habitatId", habitatId);
    prefs.putFloat("tHumidity", shared.targets.humidity);
    prefs.putFloat("tTemp", shared.targets.temperature);

    // light schedule
    prefs.putULong("light_start", shared.schedules[LED_ID_S].startSec);
    prefs.putULong("light_dur",   shared.schedules[LED_ID_S].durationSec);
    prefs.putULong("light_int",   shared.schedules[LED_ID_S].intervalSec);

    // water schedule
    prefs.putULong("water_start", shared.schedules[WATER_ID_S].startSec);
    prefs.putULong("water_dur",   shared.schedules[WATER_ID_S].durationSec);
    prefs.putULong("water_int",   shared.schedules[WATER_ID_S].intervalSec);

    xSemaphoreGive(mutex);
    prefs.end();

    Serial.println("Config saved to NVS");
}

void subscribeToTopics(void)
{
    // override
    String topic = "microgrow/"+habitatId+"/override";
    Serial.printf("Subscribing to %s", topic.c_str());
    client.subscribe(topic.c_str());
}

void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    message.reserve(length);
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.printf("MQTT [%s] => %s\n", topic, message.c_str());

    JsonDocument doc;
    auto err = deserializeJson(doc, message);
    if (err) {
        Serial.println("JSON parse error");
        return;
    }

    String tpc = String(topic);

    // -------------------------------
    // CONFIGURATION
    // -------------------------------
    if (tpc.endsWith("/init") && !initialized) {
        
        if (doc["hasID"].is<bool>) {
          hasId = true;
          habitatId = doc["id"].as<String>();
          green = doc["greenType"].as<String>();
          float tT = doc["target"]["temp"].as<float>();
          float tH = doc["target"]["humidity"].as<float>();

          if (tH < 0 || tT < 0) {
              Serial.println("Invalid config: missing fields");
              return;
          }

          xSemaphoreTake(mutex, portMAX_DELAY);
          shared.targets.humidity = tH;
          shared.targets.temperature = tT;
          shared.targets_ready = true;
          xSemaphoreGive(mutex);
        }
        else if (doc["hasSchedule"].is<bool>)
        {
          hasSchedule = true;
          unsigned long lightstartSec = doc["light"]["startTimeSec"].as<unsigned long>();
          unsigned long lightdurSec   = doc["light"]["durationSec"].as<unsigned long>();
          unsigned long lightintSec   = doc["light"]["intervalSec"].as<unsigned long>();
          unsigned long waterstartSec = doc["water"]["startTimeSec"].as<unsigned long>();
          unsigned long waterdurSec   = doc["water"]["durationSec"].as<unsigned long>();
          unsigned long waterintSec   = doc["water"]["intervalSec"].as<unsigned long>();

          xSemaphoreTake(mutex, portMAX_DELAY);
          shared.schedules[LED_ID_S].startSec = lightstartSec;
          shared.schedules[LED_ID_S].durationSec  = lightdurSec;
          shared.schedules[LED_ID_S].intervalSec  = lightintSec;
          shared.schedules[LED_ID_S].active      = true;
          shared.schedules[LED_ID_S].triggered   = false;
          shared.schedules[WATER_ID_S].startSec = waterstartSec;
          shared.schedules[WATER_ID_S].durationSec  = waterdurSec;
          shared.schedules[WATER_ID_S].intervalSec  = waterintSec;
          shared.schedules[WATER_ID_S].active      = true;
          shared.schedules[WATER_ID_S].triggered   = false;
          shared.schedules_ready = true;
          xSemaphoreGive(mutex);
        }
        Serial.println("Updated configuration:");
        Serial.printf("  Humidity: %.1f\n", tH);
        Serial.printf("  Temperature: %.1f\n", tT);
        if (hasId && hasSchedule)
        {       
          initialized = true;
          client.unsubscribe("microgrow/init");
          saveConfigToNVS();
          subscribeToTopics();
        }
        return;
    }

    if (!initialized)
      return;

    // -------------------------------
    // MANUAL OVERRIDE
    // -------------------------------
    if (tpc.endsWith("/override")) {
        int id = doc["actuator"].as<int>();
        bool enable = doc["enable"].as<bool>();

        uint8_t r, g, b;
        xSemaphoreTake(mutex, portMAX_DELAY);
        if (enable) {
            switch (id)
            {
                case FAN_ID:
                  Serial.println("Manual override for fan");
                  shared.actuators[id].manualOverride = true;
                  *((uint8_t*)(shared.actuators[id].cmdData)) = doc["value"].as<uint8_t>();
                  break;
                case WATER_PUMP_ID:
                  Serial.println("OVERRIDE: pump");
                  digitalWrite(MOTOR_PIN, HIGH);
                  vTaskDelay(pdMS_TO_TICKS(2000));
                  digitalWrite(MOTOR_PIN, LOW);
                  break;
                case LED_ID:
                  r = doc["r"].as<uint8_t>();
                  g = doc["g"].as<uint8_t>();
                  b = doc["b"].as<uint8_t>();
                  Serial.printf("OVERRIDE: led on. rgb: [%hhu], [%hhu], [%hhu]\n", r, g, b);
                  fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
                  FastLED.show();
                  break;
                default:
                  break;
            }            

            Serial.printf("Manual override enabled: actuator %d\n", id);
        } else {
            switch (id) {
                case FAN_ID:
                  shared.actuators[id].manualOverride = false;
                  shared.actuators[id].manualTriggered = false;
                  break;
                case LED_ID:
                  Serial.println("OVERRIDE: led off");
                  fill_solid(leds, NUM_LEDS, CRGB(0, 0, 0));
                  FastLED.show();
                  break;
                default:
                  break;
            }

            Serial.printf("Manual override disabled: actuator %d\n", id);
        }

        xSemaphoreGive(mutex);
        return;
    }
}

// Start WiFi configuration portal
void startPortal() {
  WiFi.disconnect(true);
  delay(100);
  
  WiFiManager wm;
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(30);
  wm.setConnectRetries(3);
  
  std::vector<const char *> menu = {"wifi"};  
  wm.setMenu(menu);
  wm.setTitle("uGrow Device Setup");
  wm.setCustomHeadElement("<style>body{font-family:sans-serif;text-align:center;}</style>");
  
  const char *customPage = 
      "<html><head><title>uGrow Setup</title></head><body>"
      "<h2>Welcome to uGrow</h2>"
      "<p>Connect this device to Wi-Fi using the button below.</p>"
      "<a href=\"/wifi\">Configure Wi-Fi</a>"
      "</body></html>";
  wm.setCustomMenuHTML(customPage);
  wm.setBreakAfterConfig(true);
  
  if (!wm.startConfigPortal("uGrow Setup")) {
    Serial.println("Portal timeout - restarting");
    ESP.restart();
  }
}

// Connect to WiFi with timeout
bool connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  
  Serial.print("Connecting to WiFi");
  uint32_t start = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - start < WIFI_TIMEOUT_MS) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  
  bool connected = WiFi.status() == WL_CONNECTED;
  if (connected) {
    Serial.printf("Connected to %s (IP: %s)\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
  } else {
    Serial.println("WiFi connection timeout");
  }
  
  return connected;
}

// Connect to MQTT broker with retries
bool connectMQTT() {
  if (client.connected()) return true;
  
  uint32_t start = millis();
  const int MAX_ATTEMPTS = 3;
  
  for (int attempt = 0; attempt < MAX_ATTEMPTS && millis() - start < MQTT_TIMEOUT_MS; attempt++) {
    String clientId = "esp32-" + String(WiFi.macAddress());
    clientId.replace(":", "");
    
    Serial.printf("MQTT attempt %d/%d...", attempt + 1, MAX_ATTEMPTS);
    
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      if (!initialized) {
        Serial.println("subscribed to init");
        client.subscribe("microgrow/init");
      } else {
        Serial.println("already initialized");
        subscribeToTopics();
      }
      // Set timezone and NTP servers
      configTime(
          -5 * 3600,   // UTC offset
          0,           // DST offset
          "pool.ntp.org",
          "time.nist.gov"
      );

      // Wait for NTP time sync
      struct tm timeinfo;
      while (!getLocalTime(&timeinfo)) {
          Serial.println("Waiting for NTP time...");
          delay(500);
      }
      return true;
    }
    
    Serial.printf("failed (rc=%d)\n", client.state());
    if (attempt < MAX_ATTEMPTS - 1) delay(2000);
  }
  
  return false;
}
bool publishData() {
    Serial.println("publishing data");
    float temp, hum, water, light;

    // Acquire shared state
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        Serial.println("Mutex timeout");
        return false;
    }

    temp  = shared.sensors[TEMPERATURE_ID].value;
    hum   = shared.sensors[HUMIDITY_ID].value;
    water = shared.sensors[WATER_LEVEL_ID].value;

    xSemaphoreGive(mutex);

    // Build topic prefixes once
    String base = "microgrow/" + habitatId + "/";

    // Publish numerical values
    if (!client.publish((base + "temp").c_str(),     String(temp).c_str()))
        return false;

    if (!client.publish((base + "humidity").c_str(), String(hum).c_str()))
        return false;

    if (!client.publish((base + "water").c_str(),    String(water).c_str()))
        return false;

    // if (!client.publish((base + "light").c_str(),    String(light).c_str()))
    //     return false;

    return true;
}


// Main MQTT task
void mqttTask(void *pv) {
  unsigned long lastWiFiAttempt = 0;
  unsigned long lastMQTTAttempt = 0;
  unsigned long lastPublish = 0;
  
  // Initial setup
  if (!connectWiFi()) {
    Serial.println("No WiFi credentials - starting portal");
    startPortal();
  }
  
  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(callback);
  client.setKeepAlive(15);
  client.setSocketTimeout(5);
  
  while (true) {
    unsigned long now = millis();
    
    // Handle portal request
    if (portalRequested) {
      Serial.println("Portal requested");
      client.disconnect();
      WiFi.disconnect();
      vTaskDelay(pdMS_TO_TICKS(100));
      startPortal();
      portalRequested = false;
      lastWiFiAttempt = lastMQTTAttempt = 0;
    }
    
    // WiFi reconnection
    if (WiFi.status() != WL_CONNECTED) {
      if (now - lastWiFiAttempt > RECONNECT_DELAY_MS) {
        Serial.println("Reconnecting WiFi...");
        connectWiFi();
        lastWiFiAttempt = now;
      }
    }
    // MQTT reconnection
    else if (!client.connected()) {
      if (now - lastMQTTAttempt > RECONNECT_DELAY_MS) {
        Serial.println("Reconnecting MQTT...");
        connectMQTT();
        lastMQTTAttempt = now;
      }
    }
    // Normal operation
    else {
      client.loop();
      
      bool ready;
      xSemaphoreTake(mutex, portMAX_DELAY);
      ready = shared.targets_ready;
      xSemaphoreGive(mutex);

      if (ready && now - lastPublish > PUBLISH_INTERVAL_MS) {
        if (publishData()) {
          lastPublish = now;
        }
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}