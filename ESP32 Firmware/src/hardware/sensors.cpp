#include "sensors.h"
#include "../config/pins.h"
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
    
    // Try an initial read to verify DHT20 is working
    delay(100);
    int status = dht20.read();
    if (status == DHT20_OK)
    {
        Serial.println("DHT20 initial read successful");
        lastDHT20Valid = true;
    }
    else
    {
        Serial.printf("DHT20 initial read failed: %d\n", status);
        lastDHT20Valid = false;
    }
    
    return true;
}

float SensorManager::readTemperature()
{
    if (!readDHT20())
    {
        Serial.println("Temperature read failed - DHT20 not valid");
        return NAN;
    }
    float temp = dht20.getTemperature() * 1.8f + 32.0f; // Convert to Fahrenheit
    Serial.printf("Temperature: %.1fF\n", temp);
    return temp;
}

float SensorManager::readHumidity()
{
    if (!readDHT20())
    {
        Serial.println("Humidity read failed - DHT20 not valid");
        return NAN;
    }
    float humid = dht20.getHumidity();
    Serial.printf("Humidity: %.1f%%\n", humid);
    return humid;
}

bool SensorManager::readWaterLevel()
{
    // Returns true if water level is LOW (needs refill)
    bool isLow = digitalRead(PIN_WATER_LEVEL) == HIGH;
    Serial.printf("Water level: %s\n", isLow ? "LOW" : "OK");
    return isLow;
}

bool SensorManager::readDHT20()
{
    uint32_t now = millis();
    
    // Don't read DHT20 more than once per 2 seconds
    if (now - lastReadTime < 2000)
    {
        Serial.printf("DHT20 cache hit (age: %lu ms) - valid: %s\n", 
                     now - lastReadTime, lastDHT20Valid ? "true" : "false");
        return lastDHT20Valid;
    }
    
    Serial.println("Reading DHT20...");
    lastReadTime = now;
    int status = dht20.read();
    
    if (status != DHT20_OK)
    {
        Serial.printf("DHT20 read failed: %d\n", status);
        lastDHT20Valid = false;
        return false;
    }
    
    Serial.println("DHT20 read successful");
    lastDHT20Valid = true;
    return true;
}

SensorReadings SensorManager::read()
{
    Serial.println("=== Reading all sensors ===");
    SensorReadings readings;
    readings.timestamp = millis();
    readings.temperature = readTemperature();
    readings.humidity = readHumidity();
    readings.waterLevelLow = readWaterLevel();
    
    readings.valid = !isnan(readings.temperature) &&
                     !isnan(readings.humidity);
    
    if (readings.valid)
    {
        Serial.printf("Sensor readings VALID - Temp: %.1fF, Humidity: %.1f%%\n", 
                     readings.temperature, readings.humidity);
        lastReadings = readings;
    }
    else
    {
        Serial.println("Sensor readings INVALID");
    }
    
    return readings;
}
