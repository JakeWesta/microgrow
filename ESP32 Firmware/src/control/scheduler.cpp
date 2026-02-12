#include "scheduler.h"
#include "../config/config.h"
#include <time.h>

// ============================================================================
// Schedule Implementation
// ============================================================================

Schedule::Schedule(const char *name)
    : name(name), state(ScheduleState::OFF), nextStartSec(0),
      activeStartTime(0) {}

void Schedule::setTiming(uint32_t startSec, uint32_t durationSec,
                         uint32_t intervalSec)
{
  this->t = Timing(startSec, durationSec, intervalSec);
}

void Schedule::setCallbacks(ScheduleCallback onStart, ScheduleCallback onEnd)
{
  this->onStart = onStart;
  this->onEnd = onEnd;
}

void Schedule::enable()
{
  if (state == ScheduleState::OFF)
  {
    state = ScheduleState::IDLE;
    nextStartSec = t.startSec;
    Serial.printf("Schedule %s: ENABLED\n", name);
  }
}

void Schedule::disable()
{
  if (state != ScheduleState::OFF)
  {
    if (state == ScheduleState::ACTIVE && onEnd)
    {
      onEnd();
    }
    state = ScheduleState::OFF;
    Serial.printf("Schedule %s: OFF\n", name);
  }
}

void Schedule::pause()
{
  if (state == ScheduleState::ACTIVE || state == ScheduleState::IDLE)
  {
    state = ScheduleState::PAUSED;
    Serial.printf("Schedule %s: PAUSED\n", name);
  }
}

void Schedule::resume()
{
  if (state == ScheduleState::PAUSED)
  {
    state = ScheduleState::ACTIVE;
    Serial.printf("Schedule %s: RESUMED\n", name);
  }
}

void Schedule::reset()
{
  if (state == ScheduleState::ACTIVE && onEnd)
  {
    onEnd();
  }
  state = ScheduleState::IDLE;
  nextStartSec = t.startSec;
  Serial.printf("Schedule %s: RESET\n", name);
}

uint32_t Schedule::getTimeRemaining() const
{
  if (state != ScheduleState::ACTIVE)
  {
    return 0;
  }

  uint32_t elapsed = Scheduler::getSecondsSinceMidnight() - activeStartTime;
  if (elapsed < t.durationSec)
  {
    return t.durationSec - elapsed;
  }
  return 0;
}

void Schedule::triggerStart()
{
  if (onStart)
  {
    onStart();
  }
  state = ScheduleState::ACTIVE;
  activeStartTime = Scheduler::getSecondsSinceMidnight();

  Serial.printf("Schedule %s: STARTED (duration=%lu sec)\n", name,
                t.durationSec);
}

void Schedule::triggerEnd()
{
  if (onEnd)
  {
    onEnd();
  }
  state = ScheduleState::IDLE;

  // Calculate next occurrence
  if (t.intervalSec > 0)
  {
    // Next start = current end time + interval
    nextStartSec = (activeStartTime + t.durationSec + t.intervalSec) % 86400UL;
  }
  else
  {
    // One-time schedule, disable after completion
    state = ScheduleState::OFF;
  }

  Serial.printf("Schedule %s: ENDED (next start=%lu)\n", name, nextStartSec);
}

void Schedule::update(uint32_t currentTimeSec)
{
  if (state == ScheduleState::OFF || state == ScheduleState::PAUSED)
  {
    return;
  }

  // Check for start condition
  if (state == ScheduleState::IDLE)
  {
    // Handle midnight rollover
    bool shouldStart = false;

    if (nextStartSec < 3600 && currentTimeSec > 82800)
    {
      // Next start is early morning, current time is late night
      shouldStart = false;
    }
    else if (currentTimeSec < 3600 && nextStartSec > 82800)
    {
      // Current time wrapped past midnight
      shouldStart = true;
    }
    else
    {
      shouldStart = (currentTimeSec >= nextStartSec);
    }

    if (shouldStart)
    {
      triggerStart();
    }
  }
  // Check for end condition
  else if (state == ScheduleState::ACTIVE)
  {
    uint32_t endTime = activeStartTime + t.durationSec;

    // Handle potential rollover
    bool shouldEnd = false;
    if (endTime >= 86400UL)
    {
      // End time goes past midnight
      uint32_t wrappedEnd = endTime % 86400UL;
      shouldEnd =
          (currentTimeSec >= wrappedEnd && activeStartTime > currentTimeSec);
    }
    else
    {
      shouldEnd = (currentTimeSec >= endTime);
    }

    if (shouldEnd)
    {
      triggerEnd();
    }
  }
}

// ============================================================================
// Scheduler Implementation
// ============================================================================

Scheduler::Scheduler()
    : lightSchedule("Light"), waterSchedule("Water"), lastUpdateTime(0) {}

void Scheduler::update()
{
  if (!isTimeValid())
  {
    return; // Don't run schedules until time is synced
  }

  uint32_t currentTime = getSecondsSinceMidnight();

  // Prevent duplicate updates in same second
  if (currentTime == lastUpdateTime)
  {
    return;
  }
  lastUpdateTime = currentTime;

  lightSchedule.update(currentTime);
  waterSchedule.update(currentTime);
}

uint32_t Scheduler::getSecondsSinceMidnight()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    return 0;
  }

  return timeinfo.tm_hour * 3600UL + timeinfo.tm_min * 60UL + timeinfo.tm_sec;
}

bool Scheduler::isTimeValid()
{
  struct tm timeinfo;
  return getLocalTime(&timeinfo);
}