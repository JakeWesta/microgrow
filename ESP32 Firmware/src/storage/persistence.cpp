#include "persistence.h"

#define READING_COUNT 24

bool PersistenceManager::isConfigured()
{
    prefs.begin(NAMESPACE, true);
    bool configured = prefs.getBool(KEY_INIT, false);
    prefs.end();
    return configured;
}

void PersistenceManager::saveConfig(const DeviceConfig &config)
{
    prefs.begin(NAMESPACE, false);
    prefs.putBool(KEY_INIT, true);
    prefs.putString(KEY_GREEN_TYPE, config.greenType);
    prefs.putString(KEY_GROWTH, config.growth);
    prefs.putFloat(KEY_TARGET_TEMP, config.targetTemp);
    prefs.putFloat(KEY_TARGET_HUM, config.targetHumidity);
    prefs.putULong(KEY_LIGHT_START, config.lightStartSec);
    prefs.putULong(KEY_LIGHT_DUR, config.lightDurationSec);
    prefs.putULong(KEY_LIGHT_INT, config.lightIntervalSec);
    prefs.putULong(KEY_WATER_START, config.waterStartSec);
    prefs.putULong(KEY_WATER_DUR, config.waterDurationSec);
    prefs.putULong(KEY_WATER_INT, config.waterIntervalSec);
    prefs.putBool(KEY_BLACKOUT, config.blackout);
    prefs.end();
    Serial.println("Configuration saved to NVS");
}

DeviceConfig PersistenceManager::loadConfig()
{
    DeviceConfig config;
    prefs.begin(NAMESPACE, true);

    if (!prefs.getBool(KEY_INIT, false))
    {
        prefs.end();
        Serial.println("No configuration found in NVS");
        return config;
    }

    config.greenType = prefs.getString(KEY_GREEN_TYPE, "");
    config.growth = prefs.getString(KEY_GROWTH, "seed");
    config.targetTemp = prefs.getFloat(KEY_TARGET_TEMP, 75.0f);
    config.targetHumidity = prefs.getFloat(KEY_TARGET_HUM, 60.0f);
    config.lightStartSec = prefs.getULong(KEY_LIGHT_START, 0);
    config.lightDurationSec = prefs.getULong(KEY_LIGHT_DUR, 0);
    config.lightIntervalSec = prefs.getULong(KEY_LIGHT_INT, 0);
    config.waterStartSec = prefs.getULong(KEY_WATER_START, 0);
    config.waterDurationSec = prefs.getULong(KEY_WATER_DUR, 0);
    config.waterIntervalSec = prefs.getULong(KEY_WATER_INT, 0);
    config.blackout = prefs.getBool(KEY_BLACKOUT, false);

    prefs.end();
    Serial.println("Configuration loaded from NVS");
    Serial.printf("  Green Type: %s, Growth: %s\n",
                  config.greenType.c_str(), config.growth.c_str());
    Serial.printf("  Targets: Temp=%.1fF, Humidity=%.1f%%\n",
                  config.targetTemp, config.targetHumidity);
    return config;
}

void PersistenceManager::clear()
{
    prefs.begin(NAMESPACE, false);
    prefs.clear();
    prefs.end();
    Serial.println("Configuration cleared from NVS");
}

//  Reading ring buffer

static void readingKey(char *buf, uint8_t slot)
{
    snprintf(buf, 5, "r%02u", slot);
}

void PersistenceManager::saveReading(const StoredReading &reading)
{
    prefs.begin(NAMESPACE, false);

    uint8_t head = prefs.getUChar(KEY_BUF_HEAD, 0);
    uint8_t count = prefs.getUChar(KEY_BUF_COUNT, 0);

    char key[5];
    readingKey(key, head);
    prefs.putBytes(key, &reading, sizeof(StoredReading));

    head = (head + 1) % READING_COUNT;
    prefs.putUChar(KEY_BUF_HEAD, head);

    if (count < READING_COUNT)
        count++;
    prefs.putUChar(KEY_BUF_COUNT, count);

    prefs.end();
    Serial.printf("Reading saved (slot %u): Temp=%.1fF, Hum=%.1f%%\n",
                  (head + READING_COUNT - 1) % READING_COUNT,
                  reading.temperature, reading.humidity);
}

StoredReading PersistenceManager::getReading(uint8_t index)
{
    StoredReading r{};
    prefs.begin(NAMESPACE, true);

    uint8_t head = prefs.getUChar(KEY_BUF_HEAD, 0);
    uint8_t count = prefs.getUChar(KEY_BUF_COUNT, 0);

    if (index >= count)
    {
        prefs.end();
        return r;
    }

    uint8_t slot = (head - count + index + READING_COUNT) % READING_COUNT;

    char key[5];
    readingKey(key, slot);
    prefs.getBytes(key, &r, sizeof(StoredReading));

    prefs.end();
    return r;
}

uint8_t PersistenceManager::getReadingCount()
{
    prefs.begin(NAMESPACE, true);
    uint8_t count = prefs.getUChar(KEY_BUF_COUNT, 0);
    prefs.end();
    return count;
}

void PersistenceManager::getAllReadings(StoredReading *out)
{
    prefs.begin(NAMESPACE, true);

    uint8_t head = prefs.getUChar(KEY_BUF_HEAD, 0);
    uint8_t count = prefs.getUChar(KEY_BUF_COUNT, 0);
    uint8_t oldest = (head - count + READING_COUNT) % READING_COUNT;

    char key[5];
    for (uint8_t i = 0; i < count; i++)
    {
        uint8_t slot = (oldest + i) % READING_COUNT;
        readingKey(key, slot);
        prefs.getBytes(key, &out[i], sizeof(StoredReading));
    }

    prefs.end();
}

void PersistenceManager::flushPendingReading(float temperature, float humidity, time_t timestamp)
{
    StoredReading r;
    r.temperature = temperature;
    r.humidity = humidity;
    r.timestamp = timestamp;
    saveReading(r);
    Serial.printf("Shutdown flush: Temp=%.1fF, Hum=%.1f%%\n", temperature, humidity);
}

void PersistenceManager::clearReadings()
{
    prefs.begin(NAMESPACE, false);
    prefs.putUChar(KEY_BUF_HEAD, 0);
    prefs.putUChar(KEY_BUF_COUNT, 0);
    char key[5];
    uint8_t blank[sizeof(StoredReading)] = {};
    for (uint8_t i = 0; i < READING_COUNT; i++)
    {
        readingKey(key, i);
        prefs.putBytes(key, blank, sizeof(StoredReading));
    }
    prefs.end();
    Serial.println("Reading history cleared");
}