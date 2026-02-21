#pragma once

#include "../hardware/sensors.h"
#include "../hardware/actuators.h"

// Target environmental parameters
struct EnvironmentalTargets
{
    float temperature; // Fahrenheit
    float humidity;    // Percentage
    bool valid;

    EnvironmentalTargets() : temperature(75.0f), humidity(60.0f), valid(false) {}
};

// Automation controller - manages environmental control
class AutomationController
{
public:
    AutomationController(SensorManager &sensors, ActuatorManager &actuators);

    // Set target parameters
    void setTargets(float temperature, float humidity);
    const EnvironmentalTargets &getTargets() const { return targets; }
    bool hasTargets() const { return targets.valid; }

    // Control loop - call periodically
    void update();

    // Enable/disable automation
    void enable() { enabled = true; }
    void disable() { enabled = false; }
    bool isEnabled() const { return enabled; }

private:
    SensorManager &sensors;
    ActuatorManager &actuators;
    EnvironmentalTargets targets;
    bool enabled;

    // Control logic
    void controlFan(const SensorReadings &readings);
    void controlMister(const SensorReadings &readings);
    void controlWater(const SensorReadings &readings);

    // Hysteresis to prevent oscillation
    uint32_t lastFanChange;
    uint32_t lastMisterChange;
    static constexpr uint32_t CONTROL_HYSTERESIS_MS = 5000; // 5 seconds
};
