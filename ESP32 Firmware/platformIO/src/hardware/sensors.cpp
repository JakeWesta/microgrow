#include "sensors.h"
#include "../config/pins.h"
#include <Wire.h>

SensorManager::SensorManager()
    : lastReadTime(0)
{
    lastReadings.valid = false;
}

bool SensorManager::begin()
{
    Wire.begin();
    dht20.begin();

    pinMode(PIN_WATER_LEVEL, INPUT);

    Serial.println("Sensors initialized");
    return true;
}

float SensorManager::readTemperature()
{
    if (!readDHT20())
    {
        return NAN;
    }
    return dht20.getTemperature() * 1.8f + 32.0f; // Convert to Fahrenheit
}

float SensorManager::readHumidity()
{
    if (!readDHT20())
    {
        return NAN;
    }
    return dht20.getHumidity();
}

bool SensorManager::readWaterLevel()
{
    // Returns true if water level is LOW (needs refill)
    return digitalRead(PIN_WATER_LEVEL) == HIGH;
}

bool SensorManager::readDHT20()
{
    uint32_t now = millis();

    // Don't read DHT20 more than once per second
    if (now - lastReadTime < 1000)
    {
        return lastReadings.valid;
    }

    lastReadTime = now;
    int status = dht20.read();

    if (status != DHT20_OK)
    {
        Serial.printf("DHT20 read failed: %d\n", status);
        return false;
    }

    return true;
}

SensorReadings SensorManager::read()
{
    SensorReadings readings;
    readings.timestamp = millis();

    readings.temperature = readTemperature();
    readings.humidity = readHumidity();
    readings.waterLevelLow = readWaterLevel();

    readings.valid = !isnan(readings.temperature) &&
                     !isnan(readings.humidity);

    if (readings.valid)
    {
        lastReadings = readings;
    }

    return readings;
}
