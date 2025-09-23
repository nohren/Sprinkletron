#include <Arduino.h>

//given a start time and interval, return true if the interval has elapsed since the start time
bool isElapsedMillis(uint32_t startTime, uint32_t interval)
{
    return (millis() - startTime) > interval;
}

// given a start time and flash interval, return true if we are in the "on" phase of the flash cycle and false for "off" phase
bool isFlashingMillis(uint32_t startTime, uint32_t flashInterval)
{
    // else, elapsed, so flash
    return ((millis() - startTime) % flashInterval) < (flashInterval / 2);
}

uint32_t nextWaterMillis(uint32_t stopTime, uint32_t waterInterval)
{
    return waterInterval - (millis() - stopTime);
}