#pragma once

#include <Arduino.h>
#include <functional>

// Schedule event handler type
using ScheduleCallback = std::function<void()>;

enum class ScheduleState
{
    IDLE,   // Waiting for start time
    ACTIVE, // Currently running
    PAUSED, // Paused due to manual override
    OFF     // Schedule OFF
};

struct Timing
{
    Timing() = default;
    Timing(uint32_t startSec, uint32_t durationSec, uint32_t intervalSec) : startSec(startSec), durationSec(durationSec), intervalSec(intervalSec) {}
    uint32_t startSec;    // First occurrence (seconds since midnight)
    uint32_t durationSec; // How long to run
    uint32_t intervalSec; // Repeat interval (0 = no repeat)
};

class Schedule
{
public:
    Schedule(const char *name);

    // Configuration
    void setTiming(uint32_t startSec, uint32_t durationSec, uint32_t intervalSec);
    void setTiming(const Timing &t) { this->t = t; }
    void setCallbacks(ScheduleCallback onStart, ScheduleCallback onEnd);
    void enable();
    void disable();

    // Control
    void pause();  // Pause due to manual override
    void resume(); // Resume from pause
    void reset();  // Reset to initial state

    // State
    bool isActive() const { return state == ScheduleState::ACTIVE; }
    bool isEnabled() const { return state != ScheduleState::OFF; }
    ScheduleState getState() const { return state; }

    // Timing info
    uint32_t getNextStartTime() const { return nextStartSec; }
    uint32_t getTimeRemaining() const;
    Timing &getTiming() { return t; }

    // Update (call periodically)
    void update(uint32_t currentTimeSec);

    const char *getName() const { return name; }

private:
    const char *name;

    // Configuration
    Timing t;

    // State
    ScheduleState state;
    uint32_t nextStartSec;    // Next scheduled start time
    uint32_t activeStartTime; // When current active period started

    // Callbacks
    ScheduleCallback onStart;
    ScheduleCallback onEnd;

    void triggerStart();
    void triggerEnd();
};

class Scheduler
{
public:
    Scheduler();

    // Initialize schedules
    Schedule &getLightSchedule() { return lightSchedule; }
    Schedule &getWaterSchedule() { return waterSchedule; }

    // Update all schedules
    void update();

    // Time management
    static uint32_t getSecondsSinceMidnight();
    static bool isTimeValid();

private:
    Schedule lightSchedule;
    Schedule waterSchedule;
    uint32_t lastUpdateTime;
};
