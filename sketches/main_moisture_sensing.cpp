// for ESP32-based moisture sensing and watering system bc of sleep feature
#ifdef ESP32
#include <Arduino.h>
#include "esp_sleep.h"

// ---- Pin map (change to match your wiring)
#define PIN_SOIL_ADC 36   // sensor analog out -> ESP32 GPIO36
#define PIN_SENSOR_PWR 19 // drives a transistor/high-side switch to power the sensor
#define PIN_PUMP_GATE 18  // goes to MOSFET gate that switches the pump
#define PIN_BUTTON 23     // push button to toggle pump on/off

// Many WROOM-32 dev boards wire the blue LED to GPIO 2.
// If LED_BUILTIN isn't defined, fall back to 2.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t PWM_CH = 0;        // 0..15
static const uint32_t PWM_FREQ = 20000; // 20 kHz (above audible range)
static const uint8_t PWM_RES_BITS = 10; // 10-bit (0..1023)
static const uint32_t SLEEP_HOURS = 24; // deep sleep interval in hours
static const uint8_t MIN_RUN_PCT = 55;
static const float MIN_MOISTURE_THRESHOLD = 2.5;     // getting drier, avoid root rot, and we check every 24 hours. It can't get too dry. We don't need to know the rate of dryness if we check more frequently.
static const float MAX_MOISTURE_THRESHOLD = 1.8;     // pretty damp
static const unsigned long FLASH_INTERVAL_MS = 1000; // for led
static const unsigned long MAX_PUMP_TIME_MS = 15000; // maximum time to run the pump in milliseconds
static const unsigned long SOFTSTART_MS = 300;       // 0.3s kick

enum class Action
{
    DRY,   // sense dry or wet soil
    WATER, // water the plant
    SLEEP  // go to deep sleep
};

// state
struct SharedState
{
    bool pumpRunning = false;         // true if pump is currently running
    unsigned long lastSampleTime = 0; // last time we sampled the sensor
    unsigned long pumpStartTime = 0;  // last time we ran the pump
    unsigned long pumpStopTime = 0;   // last time we stopped the pump
    int targetPumpPct = 80;           // target pump duty cycle in percent (0-100)
};

// ---- Function declarations
void setState(SharedState& s, Action a);
void pump(SharedState& s); // control pump based on state
void button(SharedState& s);
void sampleSoil(SharedState& s, bool verbose);
void setPumpPct(int pct); // set pump duty cycle (0-100%)
void drivePump(bool on, SharedState& s);
float getVoltage(int raw);
float getAverageVoltage(float readings[], int size);
void safeSleep(uint32_t hours);
void safeSleepSeconds(uint32_t seconds); // for testing purposes, sleeps for a number of seconds


SharedState state; // global state

// TODO
//  none

void setState(SharedState& s, Action a)
{
    switch (a)
    {
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
        safeSleep(SLEEP_HOURS); // replace with safeSleep(SLEEP_HOURS);
        break;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(1000); // for hardware stabilization on wakeup

    // Initialize pins and PWM
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_PUMP_GATE, OUTPUT);
    ledcSetup(PWM_CH, PWM_FREQ, PWM_RES_BITS);
    ledcAttachPin(PIN_PUMP_GATE, PWM_CH);

    // set ADC resolution and attenuation
    analogReadResolution(12);                        // ESP32 supports 9–12 bits (default is 12)
    analogSetPinAttenuation(PIN_SOIL_ADC, ADC_11db); // ADC_11db lets you read up to about 3.6V input
    state.lastSampleTime = millis();                 // initialize last sample time

    // Initialize pump state
    setState(state, Action::DRY); // start with pump off
}

// adding multiple sensors and functionality, no delay's
// use millis() for timing
void loop()
{
    button(state);
    sampleSoil(state, true);
    pump(state);
}

void sampleSoil(SharedState& s, bool verbose = false)
{
    static const uint8_t SAMPLE_COUNT = 10; // number of samples to average
    static uint8_t readIndex = 0;
    static uint8_t decisionCount = 30; // number of samples to decide if we need to water, this is tied to awake from sleep
    static bool monitorMode = true;
    static float lastReadings[SAMPLE_COUNT]; // store last readings for averaging

    if ((millis() - s.lastSampleTime) > 100)
    {
        int rawValue = analogRead(PIN_SOIL_ADC); // 0–4095 at 12-bit
        float voltage = getVoltage(rawValue);

        s.lastSampleTime = millis();       // update last sample time
        lastReadings[readIndex] = voltage; // store the reading
        readIndex++;
        if (readIndex >= SAMPLE_COUNT)
        {
            readIndex = 0; // wrap around
            // calculate average
            float averageVoltage = getAverageVoltage(lastReadings, SAMPLE_COUNT);
            if (verbose)
            {
                // Serial.print("Soil ADC Raw: ");
                // Serial.print(rawValue);
                Serial.print("   Voltage: ");
                Serial.print(averageVoltage, 3);
                Serial.println(" V");
            }

            // Decision making based on 30 average voltage readings
            // usually like 30 seconds of monitoring
            if (monitorMode)
            {
                decisionCount--;
                if (decisionCount == 0)
                {
                    monitorMode = false;
                    if (averageVoltage > MIN_MOISTURE_THRESHOLD)
                    {
                        setState(s, Action::WATER);
                    }
                    else
                    {
                        Serial.println("Soil moisture is within acceptable range.");
                        // go back to sleep
                        setState(s, Action::SLEEP);
                    }
                }
            }
            else
            {
                // if we are here, it means we are in water mode
                // stop watering when too moist, go to sleep
                if ((averageVoltage < MAX_MOISTURE_THRESHOLD) || ((millis() - s.pumpStartTime) > MAX_PUMP_TIME_MS))
                {
                    Serial.println("Soil moisture is too high, stopping watering.");
                    setState(s, Action::SLEEP);
                }
            }
        }
    }
}

// Convert ADC raw value to voltage
float getVoltage(int raw)
{
    return (raw / 4095.0) * 3.3;
}

float getAverageVoltage(float readings[], int size)
{
    float sum = 0;
    for (int i = 0; i < size; i++)
    {
        sum += readings[i];
    }
    return sum / size;
}

void button(SharedState& s)
{
    static uint8_t lastReading = HIGH; // for INPUT_PULLUP
    static uint8_t lastStable = HIGH;  // static variables persist across calls
    static uint32_t lastEdgeMs = 0;
    const uint32_t DEBOUNCE_MS = 50;

    uint8_t r = digitalRead(PIN_BUTTON);

    if (r != lastReading)
    {
        lastReading = r;
        lastEdgeMs = millis();
    }

    if ((millis() - lastEdgeMs) > DEBOUNCE_MS && r != lastStable)
    {
        lastStable = r;

        // Toggle exactly once on RELEASE (LOW -> HIGH)
        if (lastStable == HIGH)
        {
            if (s.pumpRunning)
                setState(s, Action::DRY);
            else
                setState(s, Action::WATER);

            Serial.println("Manual Water Toggle");
        }
    }
}

// old... kept for reference
// void button(SharedState &s) {
//     if (digitalRead(PIN_BUTTON) == LOW) {
//         s.buttonPressed = true;
//     } else {
//         if (s.buttonPressed) {
//             // debounce in here. If we see signal here within 100ms we ignore it
//             if ((millis() - s.lastButtonToggleTime) < 1000) {
//                 return;
//             }
//             Serial.println("Manual Water Toggle");
//             s.pumpRunning ? setState(s, Action::DRY) : setState(s, Action::WATER);
//             s.lastButtonToggleTime = millis(); // update last button pressed time
//         }
//         s.buttonPressed = false;
//     }
// }

void pump(SharedState& s)
{
    if (s.pumpRunning)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        drivePump(true, s);
    }
    else
    {
        if (((millis() - s.pumpStopTime) % FLASH_INTERVAL_MS) < FLASH_INTERVAL_MS / 2)
        {
            digitalWrite(LED_BUILTIN, HIGH);
        }
        else
        {
            digitalWrite(LED_BUILTIN, LOW);
        }
        drivePump(false, s);
    }
}

// Set pump duty cycle (0-100%)
void setPumpPct(int pct)
{
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    const int maxDuty = (1 << PWM_RES_BITS) - 1; // e.g., 1023
    int duty = (pct * maxDuty) / 100;
    ledcWrite(PWM_CH, duty);
}

void drivePump(bool on, SharedState& s)
{
    if (!on)
    {
        setPumpPct(0);
        return;
    }

    // clamp to a safe minimum to avoid stall
    int pct = s.targetPumpPct < MIN_RUN_PCT ? MIN_RUN_PCT : s.targetPumpPct;

    if (millis() - s.pumpStartTime < SOFTSTART_MS)
    {
        setPumpPct(100); // kick
    }
    else
    {
        setPumpPct(pct);
    }
}

// Call this instead of inlining sleep logic
void safeSleep(uint32_t hours)
{
    digitalWrite(PIN_PUMP_GATE, LOW);
    ledcWrite(PWM_CH, 0);
    digitalWrite(LED_BUILTIN, LOW);
    // 2) Log + flush
    Serial.flush();
    delay(50); // tiny settle

    // 3) Arm timer (hours -> microseconds) using integer math
    uint64_t us = (uint64_t)hours * 3600ULL * 1000000ULL;
    esp_sleep_enable_timer_wakeup(us);

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