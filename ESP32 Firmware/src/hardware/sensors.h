#pragma once

#include <Arduino.h>
#include <DHT20.h>
#include <config/pins.h>
#include <Wire.h>
#include <config/pins.h>
#include <config/config.h>

struct SensorReadings
{
    float temperature;  // Fahrenheit
    float humidity;     // Percentage
    bool waterLevelLow; // true if water needs refill
    uint32_t timestamp; // millis() when read
    bool valid;         // true if readings are valid
};

class SensorManager
{
public:
    SensorManager();
    bool begin();
    SensorReadings read();

private:
    DHT20 dht20;
    uint32_t lastReadTime;
    bool lastDHT20Valid; // Track if last DHT20 read was successful
    SensorReadings lastReadings;

    float readTemperature();
    float readHumidity();
    bool readWaterLevel();
    bool readDHT20();
};
