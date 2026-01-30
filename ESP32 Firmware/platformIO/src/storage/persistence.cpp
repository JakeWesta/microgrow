#include "persistence.h"

bool PersistenceManager::isConfigured() const
{
    Preferences p;
    p.begin(NAMESPACE, true); // read-only
    bool configured = p.getBool(KEY_INIT, false);
    p.end();
    return configured;
}

void PersistenceManager::saveConfig(const DeviceConfig &config)
{
    prefs.begin(NAMESPACE, false); // read-write

    prefs.putBool(KEY_INIT, true);
    prefs.putString(KEY_HABITAT_ID, config.habitatId);
    prefs.putString(KEY_GREEN_TYPE, config.greenType);
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

    prefs.begin(NAMESPACE, true); // read-only

    if (!prefs.getBool(KEY_INIT, false))
    {
        prefs.end();
        Serial.println("No configuration found in NVS");
        return config;
    }

    config.habitatId = prefs.getString(KEY_HABITAT_ID, "");
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
    Serial.printf("  Habitat: %s (%s)\n", config.habitatId.c_str(), config.greenType.c_str());
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

void PersistenceManager::saveTargets(float temp, float humidity)
{
    prefs.begin(NAMESPACE, false);
    prefs.putFloat(KEY_TARGET_TEMP, temp);
    prefs.putFloat(KEY_TARGET_HUM, humidity);
    prefs.end();
    Serial.printf("Targets saved: Temp=%.1fF, Humidity=%.1f%%\n", temp, humidity);
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

void PersistenceManager::saveHabitatInfo(const String &id, const String &type)
{
    prefs.begin(NAMESPACE, false);
    prefs.putString(KEY_HABITAT_ID, id);
    prefs.putString(KEY_GREEN_TYPE, type);
    prefs.end();
    Serial.printf("Habitat info saved: %s (%s)\n", id.c_str(), type.c_str());
}
