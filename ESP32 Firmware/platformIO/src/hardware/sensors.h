#pragma once

#include <Arduino.h>
#include <DHT20.h>

// Sensor readings structure
struct SensorReadings
{
    float temperature;  // Fahrenheit
    float humidity;     // Percentage
    bool waterLevelLow; // true = low water
    bool valid;         // false if readings failed
    uint32_t timestamp;
};

class SensorManager
{
public:
    SensorManager();

    bool begin();
    SensorReadings read();

    // Individual sensor access
    float readTemperature();
    float readHumidity();
    bool readWaterLevel();

    // Get last valid readings
    const SensorReadings &getLastReadings() const { return lastReadings; }

private:
    DHT20 dht20;
    SensorReadings lastReadings;
    uint32_t lastReadTime;

    bool readDHT20();
};
