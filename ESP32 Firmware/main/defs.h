#pragma once
#include <Arduino.h>

// ----- Pins -----
#define BUTTON_PIN 4
#define LED_PIN 25
#define NUM_LEDS 256
#define MOTOR_PIN 14
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 17
#define WATER_LEVEL_PIN 26
#define FAN_PIN 27

// ----- Actuators -----
#define NUM_ACTUATORS 1
#define FAN_ID 0
#define WATER_PUMP_ID 1
#define LED_ID 2

// ----- Sensors -----
#define NUM_SENSORS 3
#define TEMPERATURE_ID 0
#define HUMIDITY_ID 1
#define WATER_LEVEL_ID 2

// ----- Schedueles -----
#define NUM_SCHEDULES 2
#define WATER_ID_S 0
#define LED_ID_S 1

// ----- Actuator function pointer -----
typedef void (*ActuatorWriteFunc)(void* cmdData);

// ----- Actuator struct -----
struct Actuator {
    ActuatorWriteFunc writeFunc;
    void* cmdData;      // points to DigitalCmd, PWMCmd, etc.
    bool manualOverride;
    bool manualTriggered;
};

// ----- Sensor struct -----
typedef float (*SensorReadFunc)(void);

struct Sensor {
    SensorReadFunc readFunc;
    float value;       // latest reading
};

// ----- Targets for automation -----
struct Targets {
    float temperature;
    float humidity;
    float light;
};

// ----- Schedule for water/light -----
struct Schedule {
    unsigned long startSec;     // first start time (seconds since midnight)
    unsigned long durationSec;      // duration of event
    unsigned long intervalSec;      // repeat interval
    bool active;                   // enabled?
    void (*actionStart)(void);
    void (*actionEnd)(void);
    bool triggered;
};

// ----- Shared state -----
struct Shared {
    Sensor sensors[NUM_SENSORS];
    Actuator actuators[NUM_ACTUATORS];
    Schedule schedules[NUM_SCHEDULES];
    bool targets_ready; // targets have been configured
    bool schedules_ready; // schedules have been configured
    Targets targets;
};
