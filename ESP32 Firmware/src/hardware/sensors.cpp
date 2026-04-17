#include "sensors.h"
#include <config/pins.h>
#include <Wire.h>

SensorManager::SensorManager()
    : lastReadTime(0), lastDHT20Valid(false)
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
    float temp = dht20.getTemperature() * 1.8f + 32.0f; // Convert to Fahrenheit
    return temp;
}

float SensorManager::readHumidity()
{
    if (!readDHT20())
    {
        return NAN;
    }
    float humid = dht20.getHumidity();
    return humid;
}

bool SensorManager::readWaterLevel()
{
    // Returns true if water level is LOW (needs refill)
    bool isLow = digitalRead(PIN_WATER_LEVEL) == HIGH;
    return isLow;
}

bool SensorManager::readDHT20()
{
    uint32_t now = millis();

    // Don't read DHT20 more than once per 2 seconds
    if (now - lastReadTime < 2000)
    {
        return lastDHT20Valid;
    }

    lastReadTime = now;
    int status = dht20.read();

    if (status != DHT20_OK)
    {
        lastDHT20Valid = false;
        return false;
    }

    lastDHT20Valid = true;
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
