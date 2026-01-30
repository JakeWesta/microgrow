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
    
    // Apply hysteresis to prevent rapid cycling
    uint32_t now = millis();
    if (now - lastFanChange < CONTROL_HYSTERESIS_MS) {
        return;
    }
    
    uint8_t pwm = FAN_PWM_OFF;
    
    // Priority 1: High humidity
    if (readings.humidity > HUMIDITY_HIGH_THRESHOLD) {
        pwm = FAN_PWM_HIGH;
    }
    // Priority 2: Medium humidity
    else if (readings.humidity > HUMIDITY_MED_THRESHOLD) {
        pwm = FAN_PWM_LOW;
    }
    // Priority 3: High temperature
    else if (readings.temperature > TEMP_HIGH_THRESHOLD) {
        pwm = FAN_PWM_HIGH;
    }
    // Priority 4: Moderate temperature above target
    else if (readings.temperature > targets.temperature + 5.0f) {
        pwm = FAN_PWM_LOW;
    }
    
    fan.write(pwm);
    lastFanChange = now;
}

void AutomationController::controlMister(const SensorReadings& readings) {
    Mister& mister = actuators.getMister();
    
    // Skip if manual override is active
    if (mister.isManualOverride()) {
        return;
    }
    
    // Apply hysteresis
    uint32_t now = millis();
    if (now - lastMisterChange < CONTROL_HYSTERESIS_MS) {
        return;
    }
    
    // Turn on mister if humidity is too low
    // Add hysteresis: turn on at target-5%, turn off at target+2%
    if (readings.humidity < targets.humidity - 5.0f) {
        if (!mister.isRunning()) {
            mister.on();
            lastMisterChange = now;
        }
    } else if (readings.humidity > targets.humidity + 2.0f) {
        if (mister.isRunning()) {
            mister.off();
            lastMisterChange = now;
        }
    }
}
