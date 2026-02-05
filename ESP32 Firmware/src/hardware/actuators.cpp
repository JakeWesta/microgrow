#include "actuators.h"
#include "../config/pins.h"
#include "../config/config.h"

// ============================================================================
// Base Actuator
// ============================================================================

void Actuator::setManualOverride(bool enabled, uint8_t value)
{
    manualOverride = enabled;
    manualValue = value;
    if (enabled)
    {
        manualStartTime = millis();
        Serial.printf("%s: Manual override enabled (value=%d)\n", getName(), value);
    }
    else
    {
        Serial.printf("%s: Manual override OFF\n", getName());
    }
}

// ============================================================================
// Fan
// ============================================================================

Fan::Fan(uint8_t pin) : pin(pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void Fan::on()
{
    if (!running)
    {
        digitalWrite(pin, HIGH);
        running = true;
        Serial.println("Fan: ON");
    }
}

void Fan::off()
{
    if (running)
    {
        digitalWrite(pin, LOW);
        running = false;
        Serial.println("Fan: OFF");
    }
}

// ============================================================================
// Water Pump
// ============================================================================

WaterPump::WaterPump(uint8_t pin) : pin(pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void WaterPump::on()
{
    if (!running)
    {
        digitalWrite(pin, HIGH);
        running = true;
        Serial.println("Water Pump: ON");
    }
}

void WaterPump::off()
{
    if (running)
    {
        digitalWrite(pin, LOW);
        running = false;
        Serial.println("Water Pump: OFF");
    }
}

// ============================================================================
// Mister
// ============================================================================

Mister::Mister(uint8_t pin) : pin(pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void Mister::on()
{
    if (!running)
    {
        digitalWrite(pin, HIGH);
        running = true;
        Serial.println("Mister: ON");
    }
}

void Mister::off()
{
    if (running)
    {
        digitalWrite(pin, LOW);
        running = false;
        Serial.println("Mister: OFF");
    }
}

// ============================================================================
// LED Strip
// ============================================================================

LEDStrip::LEDStrip(uint8_t pin, uint16_t numLeds) : numLeds(numLeds)
{
    leds = new CRGB[numLeds];
    FastLED.addLeds<WS2812, PIN_LED_STRIP, GRB>(leds, numLeds);
    FastLED.setBrightness(LED_STRIP_BRIGHTNESS);
    currentColor = CRGB::Black;
    fill_solid(leds, numLeds, CRGB::Black);
    FastLED.show();
}

void LEDStrip::write(uint8_t brightness)
{
    setBrightness(brightness);
    if (brightness > 0)
    {
        setColor(CRGB::White);
    }
    else
    {
        off();
    }
}

void LEDStrip::on()
{
    setColor(currentColor);
}

void LEDStrip::setColor(uint8_t r, uint8_t g, uint8_t b)
{
    setColor(CRGB(r, g, b));
}

void LEDStrip::setColor(CRGB color)
{
    currentColor = color;
    fill_solid(leds, numLeds, color);
    FastLED.show();
    ledOn = (color != CRGB::Black);

    Serial.printf("LEDs: RGB(%d,%d,%d)%s\n",
                  color.r, color.g, color.b,
                  manualOverride ? " (manual)" : "");
}

void LEDStrip::setBrightness(uint8_t brightness)
{
    FastLED.setBrightness(brightness);
    FastLED.show();
}

void LEDStrip::off()
{
    setColor(CRGB::Black);
}

// ============================================================================
// Actuator Manager
// ============================================================================

ActuatorManager::ActuatorManager()
    : fan(PIN_FAN), pump(PIN_WATER_PUMP), mister(PIN_MISTER), leds(PIN_LED_STRIP, NUM_LEDS)
{
}

bool ActuatorManager::begin()
{
    Serial.println("Actuators initialized");
    return true;
}

void ActuatorManager::allOff()
{
    fan.off();
    pump.off();
    mister.off();
    leds.off();
    Serial.println("All actuators turned OFF");
}

void ActuatorManager::updateAll()
{
// Check for manual override timeouts
#if MANUAL_OVERRIDE_TIMEOUT_MS > 0
    uint32_t now = millis();

    Actuator *actuators[] = {&fan, &pump, &mister, &leds};
    for (auto *act : actuators)
    {
        if (act->isManualOverride())
        {
            if (now - act->manualStartTime > MANUAL_OVERRIDE_TIMEOUT_MS)
            {
                Serial.printf("%s: Manual override timeout\n", act->getName());
                act->setManualOverride(false);
            }
        }
    }
#endif
}
