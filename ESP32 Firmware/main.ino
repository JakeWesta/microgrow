//Includes MQTT code borrowed and modified from https://www.emqx.com/en/blog/esp32-connects-to-the-free-public-mqtt-broker 
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT20.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <FastLED.h>

const char *ssid = "";          // Hotspot SSID: UPDATE BEFORE FLASHING
const char *password = "";      // Hotspot Password: UPDATE BEFORE FLASHING

const char* mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;
const int BAUD_RATE = 115200;

// Pin Assignments
#define LED_PIN     33
#define NUM_LEDS    256
#define MOTOR_PIN   19
#define TFT_CS      5
#define TFT_DC      16
#define TFT_RST     17
#define WATER_LEVEL_PIN 26
// DHT20 (I2C): SDA=21, SCL=22

// Objects
DHT20 dht20;
WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
CRGB leds[NUM_LEDS];

// Variables
String habitatId = "";
unsigned long lastPublish = 0;
unsigned long lastDisplayUpdate = 0;
float currentTemp = 0;
float currentHumidity = 0;
int waterLevel = 0;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);

  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("Payload: ");
  Serial.println(message);

  if (String(topic) == "microgrow/init") {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
      return;
    }
    habitatId = doc["id"].as<String>();
    String greenType = doc["green"].as<String>();
    int schedule = doc["schedule"].as<int>();

    Serial.println("Habitat configured: ");
    Serial.println(" ID: " + habitatId);
    Serial.println(" Green Type: " + greenType);
    Serial.print(" Schedule: ");
    Serial.println(schedule);
    
    // Display on TFT
    displayHabitatInfo(habitatId, greenType, schedule);
  }
  
  // Control commands
  if (String(topic) == "microgrow/" + habitatId + "/motor") {
    if (message == "ON") {
      digitalWrite(MOTOR_PIN, HIGH);
      Serial.println("Motor ON");
    } else if (message == "OFF") {
      digitalWrite(MOTOR_PIN, LOW);
      Serial.println("Motor OFF");
    }
  }
  
  if (String(topic) == "microgrow/" + habitatId + "/led") {
    int r, g, b;
    
    sscanf(message.c_str(), "%d,%d,%d", &r, &g, &b);
    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.show();
    Serial.printf("LED set to RGB(%d,%d,%d)\n", r, g, b);
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    String clientId = "esp32-client-" + String(WiFi.macAddress());
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("microgrow/init");
      if (habitatId != "") {
        String motorTopic = "microgrow/" + habitatId + "/motor";
        String ledTopic = "microgrow/" + habitatId + "/led";
        client.subscribe(motorTopic.c_str());
        client.subscribe(ledTopic.c_str());
      }
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void displayHabitatInfo(String id, String greenType, int schedule) {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(10, 20);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.println("MicroGrow");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.println();
  tft.print("ID: ");
  tft.println(id);
  tft.print("Type: ");
  tft.println(greenType);
  tft.print("Schedule: ");
  tft.println(schedule);
}

void updateDisplay() {
  tft.fillRect(0, 100, 240, 100, ST77XX_BLACK);
  tft.setCursor(10, 100);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(1);
  tft.print("Temp: ");
  tft.print(currentTemp, 1);
  tft.println(" F");
  tft.print("Humidity: ");
  tft.print(currentHumidity, 1);
  tft.println(" %");
  tft.print("Water: ");
  tft.println(waterLevel ? "LOW" : "OK");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read sensors and publish if habitat is configured
  if (habitatId != "") {
    unsigned long now = millis();
    
    // Publish sensor data every 5 seconds
    if (now - lastPublish > 5000) {
      lastPublish = now;
      
      // Read DHT20
      int status = dht20.read();
      if (status == DHT20_OK) {
        currentTemp = dht20.getTemperature(true);
        currentHumidity = dht20.getHumidity();
        
        // Read water level
        waterLevel = digitalRead(WATER_LEVEL_PIN);
        
        // Publish temperature
        String tempTopic = "microgrow/" + habitatId + "/temperature";
        client.publish(tempTopic.c_str(), String(currentTemp).c_str());
        
        // Publish humidity
        String humidTopic = "microgrow/" + habitatId + "/humidity";
        client.publish(humidTopic.c_str(), String(currentHumidity).c_str());
        
        // Publish water level
        String waterTopic = "microgrow/" + habitatId + "/water";
        client.publish(waterTopic.c_str(), String(waterLevel).c_str());
        
        Serial.printf("Published - Temp: %.1fF, Humidity: %.1f%%, Water: %d\n", 
                      currentTemp, currentHumidity, waterLevel);
      } else {
        Serial.println("DHT20 read error!");
      }
    }
    
    // Update display every 2 seconds
    if (now - lastDisplayUpdate > 2000) {
      lastDisplayUpdate = now;
      updateDisplay();
    }
  }
}
