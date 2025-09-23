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

// Use the TFT in main.cpp
extern TFT_eSPI tft;

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
        printConfiguration(s);
        break;
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


// Cycles through configuration display items every 3 seconds in large font
void displayConfiguration(SharedState& s)
{
    static uint8_t itemIndex = 0;
    static uint32_t lastSwitchMs = 0;
    const uint32_t DISPLAY_INTERVAL_MS = 3000;

    const bool firstRun = (lastSwitchMs == 0);
    const bool timeToAdvance = (!firstRun && (millis() - lastSwitchMs >= DISPLAY_INTERVAL_MS));

    // Determine how many items are in the current mode
    uint8_t numItems = (s.mode == Mode::INTERVAL) ? 7 : (s.mode == Mode::MONITOR) ? 4 : 1; // total items per mode

    if (firstRun || timeToAdvance)
    {
        if (timeToAdvance)
        {
            itemIndex = (itemIndex + 1) % numItems;
        }
        lastSwitchMs = millis();
        // Serial.println("Displaying configuration...");
        // Serial.println(itemIndex);
        // Serial.println(numItems);

        // Prepare screen
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(tft.color565(0, 255, 65), TFT_BLACK);
        tft.setTextWrap(false);
        tft.setTextDatum(TL_DATUM); // draw from top-left to avoid baseline/datum surprises

        // Choose fonts: 4 for title, 6 for value (both are loaded in Setup25)
        const int titleFont = 4;
        const int valueFont = 4;

        String title;
        String value;
        auto fmt = [&](float v) -> String {
            int decimals = 2;
            float a = v < 0 ? -v : v;
            if (a < 0.1f) decimals = 4; else if (a < 1.0f) decimals = 3;
            char buf[24];
            dtostrf(v, 0, decimals, buf);
            return String(buf);
        };

        if (s.mode == Mode::INTERVAL)
        {
            switch (itemIndex)
            {
            case 0:
                title = "Mode";
                value = "INTERVAL";
                break;
            case 1:
                title = "Sleep (days)";
                value = fmt(s.sleepTime / (24.0f * 3600000.0f));
                break;
            case 2:
                title = "Water Interval (min)";
                value = fmt(s.waterTime / 60000.0f);
                break;
            case 3:
                title = "Tank Capacity (gal)";
                value = String(s.tankCapacityGallons, 2);
                break;
            case 4:
                title = "GPH Total";
                value = String(s.GPHTotal, 2);
                break;
            case 5:
                title = "Activations";
                value = String(s.activations);
                break;
            case 6:
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

// // Use the TFT in main.cpp
// extern TFT_eSPI tft;

// No-touch joystick-driven INIT screen
void initMode(SharedState& s) {
    static bool first = true;
    static uint32_t lastUiMs = 0;
    const uint32_t UI_REDRAW_MS = 200; // responsive

    static uint8_t selectedIndex = 0; // 0 Tank, 1 GPH, 2 Water(min), 3 Sleep(days), 4 Start
    static uint32_t lastRepeatMs = 0;
    static bool repeating = false;
    static uint8_t topIndex = 0; // first visible item in the viewport

    static bool swPrev = false;
    static uint32_t swPressMs = 0;

    // Center calibration
    static bool centerReady = false;
    static uint32_t centerAccumX = 0, centerAccumY = 0;
    static uint16_t centerX = 2048, centerY = 2048;
    static uint8_t centerSamples = 0;

    // Initialize defaults once if zeroed
    if (s.tankCapacityGallons <= 0.0f) s.tankCapacityGallons = TANK_CAPACITY_GALLONS;
    if (s.GPHTotal <= 0.0f) s.GPHTotal = GPH_TOTAL;
    if (s.waterTime == 0) s.waterTime = WATER_MS; // may be 0; user sets
    if (s.sleepTime == 0) s.sleepTime = SLEEP_MS;

    // Layout constants (landscape 240x135 typical)
    const int16_t margin = 8;
    const int16_t rowH = 36;
    const int16_t colLabelW = 120;

    if (first || (millis() - lastUiMs) > UI_REDRAW_MS) {
        first = false;
        lastUiMs = millis();
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(tft.color565(0, 255, 65), TFT_BLACK);
        tft.setTextWrap(false);
        tft.setTextDatum(TL_DATUM);

        const int titleFont = 4;
        const int valueFont = 2;
        int16_t y = margin;
        tft.drawString("Setup: Interval Mode", margin, y, titleFont);
        y += 24 + margin;

        auto drawValueRow = [&](const char* label, float value, int visRow, bool selected) {
            int16_t rowY = y + visRow * rowH;
            tft.drawString(label, margin, rowY, valueFont);
            char buf[32];
            int decimals = 2; float av = value < 0 ? -value : value;
            if (av < 0.1f) decimals = 4; else if (av < 1.0f) decimals = 3;
            dtostrf(value, 0, decimals, buf);
            String valStr = String(buf);
            int16_t valX = margin + colLabelW;
            tft.drawString(valStr, valX, rowY, valueFont);
            if (selected) {
                tft.drawString("<", valX - 12, rowY, valueFont);
                int16_t rightX = valX + tft.textWidth(valStr, valueFont) + 4;
                tft.drawString(">", rightX, rowY, valueFont);
            }
        };

        const uint8_t totalItems = 5;
        const uint8_t maxVisible = 3;
        uint8_t visibleRows = (totalItems - topIndex) < maxVisible ? (totalItems - topIndex) : maxVisible;

        for (uint8_t i = 0; i < visibleRows; i++) {
            uint8_t item = topIndex + i;
            bool selected = (item == selectedIndex);
            switch (item) {
                case 0: drawValueRow("Tank (gal)", s.tankCapacityGallons, i, selected); break;
                case 1: drawValueRow("GPH Total", s.GPHTotal, i, selected); break;
                case 2: drawValueRow("Water (min)", s.waterTime / 60000.0f, i, selected); break;
                case 3: drawValueRow("Sleep (days)", s.sleepTime / (24.0f * 3600000.0f), i, selected); break;
                case 4: {
                    int16_t rowY = y + i * rowH;
                    tft.drawString(selected ? "> Start Interval <" : "Start Interval", margin, rowY, valueFont);
                } break;
            }
        }
    }

    // Read joystick
    uint16_t rx = analogRead(PIN_JOY_X);
    uint16_t ry = analogRead(PIN_JOY_Y);
    bool swPressed = (digitalRead(PIN_JOY_SW) == LOW);

    // Calibrate center when near center to avoid skew
    if (!centerReady) {
        if (abs((int)rx - (int)centerX) < 800 && abs((int)ry - (int)centerY) < 800) {
            centerAccumX += rx;
            centerAccumY += ry;
            centerSamples++;
            if (centerSamples >= 16) {
                centerX = (uint16_t)(centerAccumX / centerSamples);
                centerY = (uint16_t)(centerAccumY / centerSamples);
                centerReady = true;
            }
        }
    }

    int dx = (int)rx - (int)centerX;
    int dy = (int)ry - (int)centerY;
    int adx = abs(dx);
    int ady = abs(dy);

    const int dead = 500;      // deadzone (try 400–600)
    const float bias = 1.2f;   // dominance bias to reject diagonals

    bool horiz = (adx > dead) && (adx > (int)(ady * bias));
    bool vert  = (ady > dead) && (ady > (int)(adx * bias));

    // Vertical navigation (dominant axis)
    if (vert) {
        bool isUp = (dy < 0);
        if (!repeating || (millis() - lastRepeatMs >= JOY_NEXT_REPEAT_MS)) {
            if (!repeating) lastRepeatMs = millis() + JOY_FIRST_REPEAT_MS - JOY_NEXT_REPEAT_MS;
            repeating = true;
            selectedIndex = isUp ? (selectedIndex == 0 ? 4 : (uint8_t)(selectedIndex - 1))
                                 : (uint8_t)((selectedIndex + 1) % 5);
            // keep selection within viewport
            const uint8_t maxVisible = 3;
            if (selectedIndex < topIndex) topIndex = selectedIndex;
            if (selectedIndex >= topIndex + maxVisible) topIndex = selectedIndex - (maxVisible - 1);
            first = true;
            lastRepeatMs = millis();
        }
    }
    // Horizontal adjustment (dominant axis)
    else if (horiz) {
        bool isRight = (dx > 0);
        if (!repeating || (millis() - lastRepeatMs >= JOY_NEXT_REPEAT_MS)) {
            if (!repeating) lastRepeatMs = millis() + JOY_FIRST_REPEAT_MS - JOY_NEXT_REPEAT_MS;
            repeating = true;
            float dir = isRight ? 1.0f : -1.0f;
            if (selectedIndex == 0) {
                s.tankCapacityGallons += 0.1f * dir;
                if (s.tankCapacityGallons < 0.1f) s.tankCapacityGallons = 0.1f;
                if (s.tankCapacityGallons > 500.0f) s.tankCapacityGallons = 500.0f;
                first = true;
            } else if (selectedIndex == 1) {
                s.GPHTotal += 0.1f * dir;
                if (s.GPHTotal < 0.1f) s.GPHTotal = 0.1f;
                if (s.GPHTotal > 1000.0f) s.GPHTotal = 1000.0f;
                first = true;
            } else if (selectedIndex == 2) {
                float waterMinutes = s.waterTime / 60000.0f;
                waterMinutes += 0.5f * dir;
                if (waterMinutes < 0.0f) waterMinutes = 0.0f;
                if (waterMinutes > 240.0f) waterMinutes = 240.0f;
                s.waterTime = (uint32_t)(waterMinutes * 60000.0f);
                first = true;
            } else if (selectedIndex == 3) {
                float sleepDays = s.sleepTime / (24.0f * 3600000.0f);
                sleepDays += 1.0f * dir; // adjust in 1-day steps
                if (sleepDays < 0.0f) sleepDays = 0.0f;
                if (sleepDays > 365.0f) sleepDays = 365.0f;
                s.sleepTime = (uint32_t)(sleepDays * 24.0f * 3600000.0f);
                first = true;
            }
            lastRepeatMs = millis();
        }
    } else {
        repeating = false;
    }

    // Switch handling: short = next/confirm, long = previous
    if (swPressed && !swPrev) {
        swPressMs = millis();
    }
    if (!swPressed && swPrev) {
        uint32_t held = millis() - swPressMs;
        if (held >= JOY_LONG_PRESS_MS) {
            selectedIndex = (selectedIndex == 0) ? 4 : (uint8_t)(selectedIndex - 1);
            const uint8_t maxVisible = 3;
            if (selectedIndex < topIndex) topIndex = selectedIndex;
            if (selectedIndex >= topIndex + maxVisible) topIndex = selectedIndex - (maxVisible - 1);
            first = true;
        } else {
            if (selectedIndex == 4) {
                if (s.tankCapacityGallons > 0.0f && s.GPHTotal > 0.0f && s.waterTime > 0U && s.sleepTime > 0U) {
                    setState(s, Action::INIT_INTERVAL);
                    return;
                }
            } else {
                selectedIndex = (selectedIndex + 1) % 5;
                const uint8_t maxVisible = 3;
                if (selectedIndex < topIndex) topIndex = selectedIndex;
                if (selectedIndex >= topIndex + maxVisible) topIndex = selectedIndex - (maxVisible - 1);
                first = true;
            }
        }
    }
    swPrev = swPressed;
}