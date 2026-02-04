#pragma once

#include <Arduino.h>
#include <Preferences.h>

struct DeviceConfig
{
    String greenType;
    float targetTemp;
    float targetHumidity;

    // Light schedule
    uint32_t lightStartSec;
    uint32_t lightDurationSec;
    uint32_t lightIntervalSec;

    // Water schedule
    uint32_t waterStartSec;
    uint32_t waterDurationSec;
    uint32_t waterIntervalSec;

    bool valid;

    DeviceConfig() : valid(false) {}
};

class PersistenceManager
{
public:
    PersistenceManager() = default;

    // Configuration
    bool isConfigured() const;
    void saveConfig(const DeviceConfig &config);
    DeviceConfig loadConfig();
    void clearConfig();

    // Individual settings
    void saveTargets(float temp, float humidity);
    void saveLightSchedule(uint32_t start, uint32_t duration, uint32_t interval);
    void saveWaterSchedule(uint32_t start, uint32_t duration, uint32_t interval);
    void saveHabitatInfo(const String &type);

private:
    Preferences prefs;
    static constexpr const char *NAMESPACE = "microgrow";

    // Keys
    static constexpr const char *KEY_INIT = "init";
    static constexpr const char *KEY_GREEN_TYPE = "greenType";
    static constexpr const char *KEY_TARGET_TEMP = "tTemp";
    static constexpr const char *KEY_TARGET_HUM = "tHumidity";
    static constexpr const char *KEY_LIGHT_START = "light_start";
    static constexpr const char *KEY_LIGHT_DUR = "light_dur";
    static constexpr const char *KEY_LIGHT_INT = "light_int";
    static constexpr const char *KEY_WATER_START = "water_start";
    static constexpr const char *KEY_WATER_DUR = "water_dur";
    static constexpr const char *KEY_WATER_INT = "water_int";
};
