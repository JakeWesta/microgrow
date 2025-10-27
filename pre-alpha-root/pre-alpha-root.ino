//Includes MQTT code borrowed and modified from https://www.emqx.com/en/blog/esp32-connects-to-the-free-public-mqtt-broker 
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char *ssid = "BrhianPhone"; // Hotspot SSID; CHANGE IF NECESSARY
const char *password = "brhian10"; // Hotspot SSID; CHANGE IF NECESSARY
const char* mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;

DHT dht(16, DHT11);

WiFiClient espClient;
PubSubClient client(espClient);

String habitatId = "";  
unsigned long lastPublish = 0;  

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
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT");
    String clientId = "esp32-client-" + String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public MQTT broker\n", clientId.c_str());
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("microgrow/init");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi!");

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (habitatId != "") {
    unsigned long now = millis();
    if (now - lastPublish > 5000) {  
      lastPublish = now;
      
      String topic = "microgrow/" + habitatId + "/sensor";
      String payload = String(dht.readTemperature() * 1.8 + 32);
      client.publish(topic.c_str(), payload.c_str());

      Serial.print("Published to ");
      Serial.print(topic);
      Serial.print(" with ");
      Serial.println(payload);
    }
  }
}
