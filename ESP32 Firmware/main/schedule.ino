#include "defs.h"
#include <time.h>
#include <FastLED.h>

extern Shared shared;
extern SemaphoreHandle_t mutex;
extern SemaphoreHandle_t led_mutex;
extern bool led_state;
extern CRGB leds[];

void setLEDColor(uint8_t r, uint8_t g, uint8_t b) {
    fill_solid(leds, NUM_LEDS, CRGB(r, g, b));
    FastLED.show();
    Serial.printf("LED set to RGB(%d, %d, %d)\n", r, g, b);
}

void ledOn() {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    led_state = false;
    xSemaphoreGive(led_mutex);
    Serial.println("led on");
    setLEDColor(255, 255, 255);
}

void ledOff() {
    xSemaphoreTake(led_mutex, portMAX_DELAY);
    led_state = true;
    xSemaphoreGive(led_mutex);
    Serial.println("led off");
    setLEDColor(0, 0, 0);
}

void pumpOn() {
    Serial.println("pump on");
    digitalWrite(MOTOR_PIN, HIGH);
}

void pumpOff() {
    Serial.println("pump off");
    digitalWrite(MOTOR_PIN, LOW);
}

unsigned long secondsSinceMidnight() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return 0;

    return
        timeinfo.tm_hour * 3600UL +
        timeinfo.tm_min  * 60UL +
        timeinfo.tm_sec;
}


void scheduleTask(void* pv) {
    bool ready = false;

    // Wait for schedules to be loaded
    do {
        xSemaphoreTake(mutex, portMAX_DELAY);
        ready = shared.schedules_ready;
        xSemaphoreGive(mutex);
        vTaskDelay(pdMS_TO_TICKS(500));
    } while (!ready);

    Serial.println("schedules ready");

    while (true) {

        unsigned long now = secondsSinceMidnight();


        for (int i = 0; i < NUM_SCHEDULES; i++) {

            xSemaphoreTake(mutex, portMAX_DELAY);
            Schedule schedule = shared.schedules[i];
            xSemaphoreGive(mutex);

            if (!schedule.active) continue;

            bool needsUpdate = false;

            // Start event
            if (!schedule.triggered && now >= schedule.startSec) {

                if (schedule.actionStart)
                    schedule.actionStart();

                schedule.triggered = true;
                needsUpdate = true;
            }

            // End event
            if (schedule.triggered &&
                now >= schedule.startSec + schedule.durationSec) {

                if (schedule.actionEnd)
                    schedule.actionEnd();

                schedule.triggered = false;

                // Move next event forward
                schedule.startSec += schedule.intervalSec;

                // Wrap at midnight
                if (schedule.startSec >= 86400UL)
                    schedule.startSec %= 86400UL;

                needsUpdate = true;
            }

            if (needsUpdate) {
                xSemaphoreTake(mutex, portMAX_DELAY);
                shared.schedules[i] = schedule;
                xSemaphoreGive(mutex);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}