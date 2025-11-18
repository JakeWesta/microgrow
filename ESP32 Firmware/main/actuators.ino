#include <Arduino.h>
#include "defs.h"

extern Shared shared;
extern SemaphoreHandle_t mutex;

void writeFan(void *arg) {
    uint8_t pwm = *((uint8_t*)arg);
    digitalWrite(FAN_PIN, pwm?HIGH:LOW);
}

void fillAutoCommand(int index, void *cmdData) {
    // Only handle fan for now
    if (index != FAN_ID)
        return;

    uint8_t *pwm = (uint8_t*)cmdData;
    float temp;
    float hum;
    float targetHumidity, targetTemp;
    // Read sensors safely
    if (xSemaphoreTake(mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        temp = shared.sensors[TEMPERATURE_ID].value;
        hum  = shared.sensors[HUMIDITY_ID].value;
        targetHumidity = shared.targets.humidity;
        targetTemp = shared.targets.temperature;
        xSemaphoreGive(mutex);
    } else {
        return;
    }

    // Calculate PWM (no mutex needed here)
    if (hum > 70.0f) {
        *pwm = 255;
    } 
    else if (hum > 60.0f) {
        *pwm = 128;
    } 
    else if (temp > 85.0f) {
        *pwm = 200;
    } 
    else {
        *pwm = 0;
    }
}

void actuatorTask(void* pv) {
    bool ready = false;
    do {
        xSemaphoreTake(mutex, portMAX_DELAY);
        ready = shared.targets_ready;
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(100));
    } while (!ready);

    Serial.println("targets ready");

    while (true) {
        for (int i = 0; i < NUM_ACTUATORS; i++) {
            bool manual, triggered;
            void* cmdData;
            void (*writeFunc)(void*);

            // Read shared actuator state
            if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                manual = shared.actuators[i].manualOverride;
                triggered = shared.actuators[i].manualTriggered;
                cmdData = shared.actuators[i].cmdData;
                writeFunc = shared.actuators[i].writeFunc;
                xSemaphoreGive(mutex);
            } else {
                Serial.println("Mutex timeout in actuator task");
                continue;
            }

            // Compute command if not in manual mode
            if (!manual) {
                fillAutoCommand(i, cmdData);
            }

            // Apply command
            if (writeFunc != nullptr && !triggered) {
                writeFunc(cmdData);
            }

            if (manual)
            {
                if (xSemaphoreTake(mutex, pdMS_TO_TICKS(100) == pdTRUE)) {
                    shared.actuators[i].manualTriggered = true;
                    xSemaphoreGive(mutex);
                }
                else continue;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}