#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "Buttons.h"
#include "Intervals.h"
#ifdef ESP32
#include <esp_sleep.h>
#include "Pwm.h"
// Call this instead of inlining sleep logic
void safeSleep()
{
    digitalWrite(PIN_PUMP_GATE, LOW);
    ledcWrite(PWM_CH, 0);
    digitalWrite(LED_BUILTIN, LOW);
    // 2) Log + flush
    Serial.flush();
    delay(50); // tiny settle

    // 3) Arm timer (hours -> microseconds) using integer math
    esp_sleep_enable_timer_wakeup(SLEEP_US);

    // 4) Sleep (never returns)
    esp_deep_sleep_start();

}

// for testing
void safeSleepSeconds(uint32_t seconds)
{
    ledcWrite(PWM_CH, 0);
    digitalWrite(PIN_PUMP_GATE, LOW);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.println("Sleeping (seconds)...");
    Serial.flush();
    uint64_t us = (uint64_t)seconds * 1000000ULL;
    esp_sleep_enable_timer_wakeup(us);
    esp_deep_sleep_start();
}
#endif

uint32_t totalActivations(float tankCapacityGallons, float gph, uint32_t waterIntervalMs) {
    if (gph <= 0.0f || waterIntervalMs == 0) return 0;

    const float gallonsPerAct = (gph / 3600000.0f) * waterIntervalMs;
    if (gallonsPerAct <= 0.0f) return 0;

    const float raw = tankCapacityGallons / gallonsPerAct;
    return (raw > 0.0f) ? (uint32_t)floorf(raw) : 0;
}


void setState(SharedState& s, Action a)
{
    switch (a)
    {
    case Action::INIT_SETUP:
        s.mode = Mode::INIT; // default mode
        s.pumpRunning = false;
        break;
    case Action::INIT_INTERVAL:
        s.mode = Mode::INTERVAL;
        s.pumpStopTime = millis();
        s.maxActivations = totalActivations(s.tankCapacityGallons, s.GPHTotal, s.waterTime);
        Serial.print("Activation limit: ");
        Serial.println(s.maxActivations);
    case Action::INIT_MONITOR:
        s.mode = Mode::MONITOR;
        s.pumpRunning = false;
        s.pumpStopTime = millis();
        s.lastSampleTime = millis();
        break;
    case Action::DRY:
        if (!s.pumpRunning)
        {
            Serial.println("Pump is not running.");
        }
        else
        {
            Serial.println("Stopping pump...");
            s.pumpRunning = false; // set pump running state
            s.pumpStopTime = millis();
        }
        break;
    case Action::WATER:
        // water the plant
        if (s.pumpRunning)
        {
            Serial.println("Pump is already running.");
        }
        else
        {
            Serial.println("Starting pump...");
            s.pumpRunning = true;       // set pump running state
            s.pumpStartTime = millis(); // reset pump start time
        }
        break;
    case Action::SLEEP:
        if (s.pumpRunning)
        {
            s.pumpRunning = false;
            s.pumpStopTime = millis();
        }
        Serial.println("Going to sleep...");
        safeSleep();
        break;
    }
}

void button(SharedState& s)
{
    if (isButtonReleased(PIN_BUTTON))
    {
        if (s.pumpRunning)
            setState(s, Action::DRY);
        else
            setState(s, Action::WATER);

        Serial.println("Manual Water Toggle");
    }
}

void printConfiguration(SharedState& s) {
    if (s.mode == Mode::INTERVAL) {
        Serial.println(" Mode = INTERVAL ");
        Serial.print("  Water Interval (min): ");
        Serial.println(s.waterTime / 60000.0); //ms to minutes
        Serial.print("  Tank Capacity (gallons): ");
        Serial.println(s.tankCapacityGallons);
        Serial.print("  GPH Total: ");
        Serial.println(s.GPHTotal);
        Serial.println(" Activations: ");
        Serial.println(s.activations);
        Serial.print("  Max Activations: ");
        Serial.println(s.maxActivations);
    }
    else if (s.mode == Mode::MONITOR) {
        Serial.println(" Mode = MONITOR ");
        Serial.print("  Min Moisture Threshold: ");
        Serial.println(MIN_MOISTURE_THRESHOLD);
        Serial.print("  Max Moisture Threshold: ");
        Serial.println(MAX_MOISTURE_THRESHOLD);
        Serial.print("  Monitor Frequency: ");
        Serial.println(s.sleepTime / 60000.0); // ms to minutes
    }
}
// (biz logic) waters at start of interval. Counts activations and stops when limit is reached.
void intervalMode(SharedState& s) {
    if (!s.pumpRunning) {
        if (isElapsedMillis(s.pumpStopTime, s.sleepTime)) {
            if (s.activations < s.maxActivations) {
                setState(s, Action::WATER);
                s.activations++;
                Serial.print("Activations: ");
                Serial.println(s.activations);
                Serial.print("Activation limit: ");
                Serial.println(s.maxActivations);
            }
            else {
                Serial.println("Activation limit reached, refill water and toggle on off to reset.");
                if (isFlashingMillis(s.pumpStopTime, FLASH_INTERVAL_MS)) {
                    digitalWrite(LED_BUILTIN, HIGH);
                    digitalWrite(PIN_LED_LARGE, HIGH);
                }
                else {
                    digitalWrite(LED_BUILTIN, LOW);
                    digitalWrite(PIN_LED_LARGE, LOW);
                }
            }
        }
    }
    else {
        if (isElapsedMillis(s.pumpStartTime, s.waterTime)) {
            Serial.println("Water interval complete, stopping pump.");
            setState(s, Action::DRY);
        }
    }
}

void monitorMode(SharedState& s)
{
    sampleSoil(s, true);
}

void pump(SharedState& s)
{
    if (s.pumpRunning)
    {
#ifdef ESP32
        drivePump(true, s.targetPumpPct, MIN_RUN_PCT, s.pumpStartTime, SOFTSTART_MS, PWM_CH, PWM_RES_BITS);
#else
        digitalWrite(PIN_PUMP_GATE, HIGH);
#endif

        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(PIN_LED_LARGE, HIGH);
    }
    else
    {
#ifdef ESP32
        drivePump(false, s.targetPumpPct, MIN_RUN_PCT, s.pumpStartTime, SOFTSTART_MS, PWM_CH, PWM_RES_BITS);
#else
        digitalWrite(PIN_PUMP_GATE, LOW);
#endif
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(PIN_LED_LARGE, LOW);
    }
}

// Use the TFT in main.cpp
extern TFT_eSPI tft;

// Cycles through configuration display items every 3 seconds in large font
void displayConfiguration(SharedState& s)
{
    static uint8_t itemIndex = 0;
    static uint32_t lastSwitchMs = 0;
    const uint32_t DISPLAY_INTERVAL_MS = 3000;

    const bool firstRun = (lastSwitchMs == 0);
    const bool timeToAdvance = (!firstRun && (millis() - lastSwitchMs >= DISPLAY_INTERVAL_MS));

    // Determine how many items are in the current mode
    uint8_t numItems = (s.mode == Mode::INTERVAL) ? 6 : (s.mode == Mode::MONITOR) ? 4 : 1; // total items per mode

    if (firstRun || timeToAdvance)
    {
        if (timeToAdvance)
        {
            itemIndex = (itemIndex + 1) % numItems;
        }
        lastSwitchMs = millis();
        Serial.println("Displaying configuration...");
        Serial.println(itemIndex);
        Serial.println(numItems);

        // Prepare screen
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextWrap(false);
        tft.setTextDatum(TL_DATUM); // draw from top-left to avoid baseline/datum surprises

        // Choose fonts: 4 for title, 6 for value (both are loaded in Setup25)
        const int titleFont = 4;
        const int valueFont = 4;

        String title;
        String value;

        if (s.mode == Mode::INTERVAL)
        {
            switch (itemIndex)
            {
            case 0:
                title = "Mode";
                value = "INTERVAL";
                break;
            case 1:
                title = "Water Interval (min)";
                value = String(s.waterTime / 60000.0f, 2);
                break;
            case 2:
                title = "Tank Capacity (gal)";
                value = String(s.tankCapacityGallons, 2);
                break;
            case 3:
                title = "GPH Total";
                value = String(s.GPHTotal, 2);
                break;
            case 4:
                title = "Activations";
                value = String(s.activations);
                break;
            case 5:
                title = "Max Activations";
                value = String(s.maxActivations);
                break;
            }
        }
        else if (s.mode == Mode::MONITOR) // Mode::MONITOR
        {
            switch (itemIndex)
            {
            case 0:
                title = "Mode";
                value = "MONITOR";
                break;
            case 1:
                title = "Min Moisture Thresh";
                value = String(MIN_MOISTURE_THRESHOLD, 2);
                break;
            case 2:
                title = "Max Moisture Thresh";
                value = String(MAX_MOISTURE_THRESHOLD, 2);
                break;
            case 3:
                title = "Monitor Freq (min)";
                value = String(s.sleepTime / 60000.0f, 2);
                break;
            }
        } else if (s.mode == Mode::INIT) {
            switch (itemIndex) {
                case 0:
                    title = "Mode";
                    value = "INIT";
                    break;
            }
        } else {
            switch (itemIndex) {
                case 0:
                    title = "Mode";
                    value = "UNKNOWN";
                    break;
            }
        }

        // Compute positions (landscape 240x135 when rotation=1)
        const int16_t screenW = tft.width();
        const int16_t screenH = tft.height();

        const int16_t titleH = tft.fontHeight(titleFont);
        const int16_t valueH = tft.fontHeight(valueFont);

        const int16_t marginX = 8;
        const int16_t marginY = 8;

        const int16_t titleX = marginX;
        const int16_t titleY = marginY + (screenH / 6); // a little down from the top

        const int16_t valueY = titleY + titleH + 12;
        int16_t valueX = (screenW - tft.textWidth(value, valueFont)) / 2;
        if (valueX < 0) valueX = 0; // clamp to left edge if too wide

        tft.drawString(title + ":", titleX, titleY, titleFont);
        tft.drawString(value, valueX, valueY, valueFont);
    }
}