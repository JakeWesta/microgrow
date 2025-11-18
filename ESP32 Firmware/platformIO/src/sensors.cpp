#include <Arduino.h>
#include <DHT20.h>
#include "defs.h"

// Extern references to shared state and mutex
extern Shared shared;
extern SemaphoreHandle_t mutex;
extern DHT20 dht20;
bool status;


float readTemperature(void) {
    status = dht20.read();
    if (status != DHT20_OK) return NAN;

    return dht20.getTemperature() * 1.8 + 32;
}

float readHumidity(void) {
    if (status != DHT20_OK) return NAN;

    return dht20.getHumidity();
}

float readWaterLevel(void) {
    return (float)digitalRead(WATER_LEVEL_PIN);
}

// Sensor task
void sensorTask(void* pv) {
    bool ready = false;
    do {
        xSemaphoreTake(mutex, portMAX_DELAY);
        ready = shared.targets_ready;
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(500));
    } while (!ready);

    Serial.println("Starting sensors");

    while (true) {
        for (int i = 0; i < NUM_SENSORS; i++) {
            float val = shared.sensors[i].readFunc();

            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                shared.sensors[i].value = val;
                xSemaphoreGive(mutex);
            } else {
                Serial.println("Mutex timeout in sensorTask");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
