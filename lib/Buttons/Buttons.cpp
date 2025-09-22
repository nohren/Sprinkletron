#include <Arduino.h>

bool isButtonReleased(int pin, bool armed = true)
{
    static uint8_t lastReading = HIGH; // for INPUT_PULLUP
    static uint8_t lastStable = HIGH;  // static variables persist across calls
    static uint32_t lastEdgeMs = 0;
    const uint32_t DEBOUNCE_MS = 50;
    uint8_t r = digitalRead(pin);

    if (r != lastReading)
    {
        lastReading = r;
        lastEdgeMs = millis();
    }

    if ((millis() - lastEdgeMs) > DEBOUNCE_MS && r != lastStable)
    {
        lastStable = r;
        // Toggle exactly once on RELEASE (LOW -> HIGH)
        if (lastStable == HIGH && armed == true)
        {
            return true;
        }
    }
    return false;
}