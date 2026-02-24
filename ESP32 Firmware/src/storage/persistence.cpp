#include "persistence.h"

#define READING_COUNT 3

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
    config.targetTemp = prefs.getFloat(KEY_TARGET_TEMP, 75.0f);
    config.targetHumidity = prefs.getFloat(KEY_TARGET_HUM, 60.0f);
    config.lightStartSec = prefs.getULong(KEY_LIGHT_START, 0);
    config.lightDurationSec = prefs.getULong(KEY_LIGHT_DUR, 0);
    config.lightIntervalSec = prefs.getULong(KEY_LIGHT_INT, 0);
    config.waterStartSec = prefs.getULong(KEY_WATER_START, 0);
    config.waterDurationSec = prefs.getULong(KEY_WATER_DUR, 0);
    config.waterIntervalSec = prefs.getULong(KEY_WATER_INT, 0);
    config.valid = true;

    prefs.end();
    Serial.println("Configuration loaded from NVS");
    Serial.printf("  Green Type: %s\n", config.greenType.c_str());
    Serial.printf("  Targets: Temp=%.1fF, Humidity=%.1f%%\n",
                  config.targetTemp, config.targetHumidity);
    return config;
}

void PersistenceManager::clearConfig()
{
    prefs.begin(NAMESPACE, false);
    prefs.clear();
    prefs.end();
    Serial.println("Configuration cleared from NVS");
}

void PersistenceManager::saveTargets(float temperature, float humidity)
{
    prefs.begin(NAMESPACE, false);
    prefs.putFloat(KEY_TARGET_TEMP, temperature);
    prefs.putFloat(KEY_TARGET_HUM, humidity);
    prefs.end();
    Serial.printf("Targets saved: Temp=%.1fF, Humidity=%.1f%%\n", temperature, humidity);
}

void PersistenceManager::saveLightSchedule(uint32_t start, uint32_t duration, uint32_t interval)
{
    prefs.begin(NAMESPACE, false);
    prefs.putULong(KEY_LIGHT_START, start);
    prefs.putULong(KEY_LIGHT_DUR, duration);
    prefs.putULong(KEY_LIGHT_INT, interval);
    prefs.end();
    Serial.printf("Light schedule saved: start=%lu, dur=%lu, int=%lu\n",
                  start, duration, interval);
}

void PersistenceManager::saveWaterSchedule(uint32_t start, uint32_t duration, uint32_t interval)
{
    prefs.begin(NAMESPACE, false);
    prefs.putULong(KEY_WATER_START, start);
    prefs.putULong(KEY_WATER_DUR, duration);
    prefs.putULong(KEY_WATER_INT, interval);
    prefs.end();
    Serial.printf("Water schedule saved: start=%lu, dur=%lu, int=%lu\n",
                  start, duration, interval);
}

void PersistenceManager::saveHabitatInfo(const String &type)
{
    prefs.begin(NAMESPACE, false);
    prefs.putString(KEY_GREEN_TYPE, type);
    prefs.end();
    Serial.printf("Habitat info saved: %s\n", type.c_str());
}

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