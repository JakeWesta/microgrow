#pragma once

#include <Arduino.h>
#include <FastLED.h>

// Base actuator class
class Actuator
{
public:
    virtual ~Actuator() = default;

    virtual void on() = 0;
    virtual void off() = 0;
    virtual const char *getName() const = 0;

    // Manual override management
    void setManualOverride(bool enabled) { manualOverride = enabled; }
    bool isManualOverride() const { return manualOverride; }

protected:
    bool manualOverride = false;

    friend class ActuatorManager;
};

// Fan control (PWM)
class Fan : public Actuator
{
public:
    Fan(uint8_t pin);
    void on();
    void off() override;
    const char *getName() const override { return "Fan"; }
    bool isRunning() const { return running; }

private:
    uint8_t pin;
    bool running = false;
};

// Water pump (on/off)
class WaterPump : public Actuator
{
public:
    WaterPump(uint8_t pin);
    void on() override;
    void off() override;
    const char *getName() const override { return "Water Pump"; }

    bool isRunning() const { return running; }

private:
    uint8_t pin;
    bool running = false;
};

// Mister (on/off)
class Mister : public Actuator
{
public:
    Mister(uint8_t pin);
    void on() override;
    void off() override;
    const char *getName() const override { return "Mister"; }

    bool isRunning() const { return running; }

private:
    uint8_t pin;
    bool running = false;
};

// LED strip (RGB control)
class LEDStrip : public Actuator
{
public:
    LEDStrip(uint8_t pin, uint16_t numLeds);

    void write(uint8_t brightness); // 0-255
    void on() override;
    void off() override;
    void flash(CRGB color);
    const char *getName() const override { return "LED Strip"; }

    void setColor(uint8_t r, uint8_t g, uint8_t b);
    void setColor(CRGB color);
    void setBrightness(uint8_t brightness);

    bool isOn() const { return ledOn; }

private:
    CRGB *leds;
    uint16_t numLeds;
    CRGB currentColor;
    bool ledOn = false;
};

// Actuator manager - controls all actuators
class ActuatorManager
{
public:
    ActuatorManager();

    bool begin();

    // Access individual actuators
    Fan &getFan() { return fan; }
    WaterPump &getPump() { return pump; }
    Mister &getMister() { return mister; }
    LEDStrip &getLEDs() { return leds; }

    // Bulk operations
    void allOff();

private:
    Fan fan;
    WaterPump pump;
    Mister mister;
    LEDStrip leds;
};
