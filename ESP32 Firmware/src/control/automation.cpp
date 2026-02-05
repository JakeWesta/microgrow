#include "automation.h"
#include "../config/config.h"

AutomationController::AutomationController(SensorManager& sensors, ActuatorManager& actuators)
    : sensors(sensors)
    , actuators(actuators)
    , enabled(false)
    , lastFanChange(0)
    , lastMisterChange(0) {
}

void AutomationController::setTargets(float temperature, float humidity) {
    targets.temperature = temperature;
    targets.humidity = humidity;
    targets.valid = true;
    
    Serial.printf("Targets set: Temp=%.1fF, Humidity=%.1f%%\n", 
                  temperature, humidity);
}

void AutomationController::update() {
    if (!enabled || !targets.valid) {
        return;
    }
    
    SensorReadings readings = sensors.read();
    if (!readings.valid) {
        Serial.println("Automation: Invalid sensor readings");
        return;
    }
    
    // Control actuators based on sensor readings
    controlFan(readings);
    controlMister(readings);
}

void AutomationController::controlFan(const SensorReadings& readings) {
    Fan& fan = actuators.getFan();
    
    // Skip if manual override is active
    if (fan.isManualOverride()) {
        return;
    }
    
    uint32_t now = millis();
    if (now - lastFanChange < CONTROL_HYSTERESIS_MS) {
        return;
    }
    
    // Turn on fan if temperature or humidity is too high
    if (readings.temperature > targets.temperature || 
        readings.humidity > targets.humidity) {
        if (!fan.isRunning()) {
            fan.on();
            lastFanChange = now;
        }
    } else if (readings.temperature < targets.temperature - 2.0f && 
               readings.humidity < targets.humidity - 5.0f) {
        // Turn off fan when both are sufficiently low
        if (fan.isRunning()) {
            fan.off();
            lastFanChange = now;
        }
    }
}


void AutomationController::controlMister(const SensorReadings& readings) {
    Mister& mister = actuators.getMister();
    
    // Skip if manual override is active
    if (mister.isManualOverride()) {
        return;
    }
    
    uint32_t now = millis();
    if (now - lastMisterChange < CONTROL_HYSTERESIS_MS) {
        return;
    }
    
    // NOTE: Not correct logic for DEMO
    if (readings.humidity < 1/*targets.humidity - 5.0f*/) {
        // If humidity is sufficiently low, turn on mister
        if (!mister.isRunning()) {
            mister.on();
            lastMisterChange = now;
        }
    } else if (readings.humidity > 1/*targets.humidity + 2.0f*/) {
        // If humidity is sufficiently high, turn off mister
        if (mister.isRunning()) {
            mister.off();
            lastMisterChange = now;
        }
    }
}
