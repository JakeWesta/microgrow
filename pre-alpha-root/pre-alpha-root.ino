//Includes MQTT code borrowed and modified from https://www.emqx.com/en/blog/esp32-connects-to-the-free-public-mqtt-broker 
#include <PubSubClient.h>
#include <WiFi.h>
#include <string>
const int PIN_D21 = 21;
const int PIN_D4 = 4;
const int PIN_D34 = 34;

const char* ssid = "";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";  //public MQTT broker
const char *mqtt_broker = "broker.emqx.io";
const char *topic = "microgrow/esp32_data";
const char *mqtt_username = "microgrow";
const char *mqtt_password = "public";
const int mqtt_port = 1883;
WiFiClient espClient;
PubSubClient client(espClient);

int ADC_VAL = -101; //temp for comparison, guarantees first is always sent
int temp = 0;

void setup() {
  pinMode(PIN_D4, OUTPUT); //setup analog D4
  pinMode(PIN_D21, OUTPUT);
  Serial.begin(115200);

  if(!ledcAttach(PIN_D21, 5000, 8)){      
    Serial.println("PWM FAILED");    
    } //setup D21

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("Connected to WiFi.");
  //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    while (!client.connected()) {
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    // Publish and subscribe
    client.publish(topic, "This is the MicroGrow ESP32 :)");
    client.subscribe(topic);
}
void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}
void loop() {
  client.loop();

  temp = analogRead(PIN_D34); //read d15 for ADC value
  if (abs(temp - ADC_VAL) > 100 || ADC_VAL == -101){
    ADC_VAL = temp;
    char msgBuffer[128];
    sprintf(msgBuffer, "%d", ADC_VAL); //send read-in ADC value to BROKER
    Serial.println(String(ADC_VAL));
    client.publish(topic, msgBuffer);
  }
  digitalWrite(PIN_D4, !digitalRead(PIN_D4));  //toggle pin
  for (int i = 255; i >= 0; i--) {            //Fade LED on and off
    ledcWrite(PIN_D21, i);
    delay(2);
  }
  for (int i = 0; i <= 255; i++) {
    ledcWrite(PIN_D21, i);
    delay(2);
  }
}
