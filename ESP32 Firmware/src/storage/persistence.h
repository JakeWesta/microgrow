#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

struct StoredReading
{
    float temperature;
    float humidity;
    time_t timestamp;
    StoredReading() : temperature(0), humidity(0), timestamp(0) {}
};

struct DeviceConfig
{
    String greenType;
    String growth;
    float targetTemp;
    float targetHumidity;
    uint32_t lightStartSec;
    uint32_t lightDurationSec;
    uint32_t lightIntervalSec;
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
    bool isConfigured();
    void saveConfig(const DeviceConfig &config);
    DeviceConfig loadConfig();
    void clearConfig();

    // Readings
    void saveReading(const StoredReading &reading);
    StoredReading getReading(uint8_t index);
    uint8_t getReadingCount();
    void getAllReadings(StoredReading *out);
    void clearReadings();

    // Individual settings
    void saveTargets(float temp, float humidity);
    void saveLightSchedule(uint32_t start, uint32_t duration, uint32_t interval);
    void saveWaterSchedule(uint32_t start, uint32_t duration, uint32_t interval);
    void saveHabitatInfo(const String &type);

private:
    Preferences prefs;
    static constexpr const char *NAMESPACE = "microgrow";
    static constexpr const char *KEY_INIT = "init";
    static constexpr const char *KEY_GREEN_TYPE = "greenType";
    static constexpr const char *KEY_GROWTH = "growth";
    static constexpr const char *KEY_TARGET_TEMP = "tTemp";
    static constexpr const char *KEY_TARGET_HUM = "tHumidity";
    static constexpr const char *KEY_LIGHT_START = "light_start";
    static constexpr const char *KEY_LIGHT_DUR = "light_dur";
    static constexpr const char *KEY_LIGHT_INT = "light_int";
    static constexpr const char *KEY_WATER_START = "water_start";
    static constexpr const char *KEY_WATER_DUR = "water_dur";
    static constexpr const char *KEY_WATER_INT = "water_int";
    static constexpr const char *KEY_BUF_HEAD = "buf_head";
    static constexpr const char *KEY_BUF_COUNT = "buf_count";
};