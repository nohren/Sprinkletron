#include <Arduino.h>

//given a start time and interval, return true if the interval has elapsed since the start time
bool isElapsedMillis(uint32_t startTime, uint32_t interval)
{
    return (millis() - startTime) > interval;
}

// given a start time and flash interval, return true if we are in the "on" phase of the flash cycle and false for "off" phase
bool isFlashingMillis(uint32_t flashInterval)
{
    // else, elapsed, so flash
    return millis() % (2 * flashInterval) < flashInterval;
}

uint32_t countdownMillis(uint32_t startMs, uint32_t durationMs) {
    uint32_t now = millis();
    uint32_t elapsed = now - startMs;
    if (elapsed >= durationMs) return 0;
    return durationMs - elapsed;
}

String convertMillisToHHMMSS(uint32_t ms) {
    uint32_t hh = ms / 3600000UL;
    uint32_t mm = (ms % 3600000UL) / 60000UL;
    uint32_t ss = (ms % 60000UL) / 1000UL;
    char buf[10]; // needs at least 9; give a little headroom
    snprintf(buf, sizeof(buf), "%02u:%02u:%02u", (unsigned)hh, (unsigned)mm, (unsigned)ss);
    return String(buf);
}

String convertMillisToDays(uint32_t ms) {
    uint32_t days = ms / 86400000UL;
    return String(days) + " days";
}