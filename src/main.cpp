#include <Arduino.h>
#include "Intervals.h"
#include "Buttons.h"
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#define PIN_PUMP_GATE 3
#define PIN_BUTTON 5
#define RED_PIN 10
#define GREEN_PIN 9
#define BLUE_PIN 6

//NEVER USE A3!!! IT IS CONNECTED TO GROUND

struct SharedState
{
    bool pumpRunning = false;   // true if pump is currently running
    bool buttonActivated = false; // true if button is currently pressed
    uint32_t pumpStartTime = 0;  // last time we ran the pump
    uint32_t pumpStopTime = 0;   // last time we stopped the pump // maximum number of activations before water runs out
    uint32_t activations = 0;
};

enum class Action
{
    INIT,
    DRY,   // sense dry or wet soil
    WATER, // water the plant
};

static const uint32_t FLASH_INTERVAL_MS = 1000; // for led
static const uint32_t ERROR_FLASH_INTERVAL_MS = 500; // for led
static const uint32_t WATER_MS = 45000; // 45 seconds
static const uint32_t DAY_MS = 86400000; // 1 day
static const uint32_t SLEEP_MS = 5 * DAY_MS; // 5 days
static const float TANK_CAPACITY_GALLONS = 2.905; //11 liter tank has 2.905 gallons
static const float GPH_TOTAL = 4; // total galldon per hour heads on the
void printConfiguration(SharedState& s);
void setState(SharedState& s, Action a);
void intervalMode(SharedState& s);
void pump(SharedState& s); // control pump based on state
uint32_t totalActivations(float tankCapacityGallons, float gph, uint32_t waterIntervalMs);
void button(SharedState& s);
void setRGB(uint8_t r, uint8_t g, uint8_t b);
void updateGradient(uint32_t now, uint32_t intervalMs, uint32_t lastWaterStop, uint32_t stepMs = 100);
static const uint32_t MAX_ACTIVATIONS = totalActivations(TANK_CAPACITY_GALLONS, GPH_TOTAL, WATER_MS);
const bool COMMON_ANODE = false;  // likely true based on your symptom

SharedState state;

void setup() {
    Serial.begin(115200);
    delay(1000); // for hardware stabilization on wakeup
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_PUMP_GATE, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(RED_PIN, OUTPUT);
    pinMode(BLUE_PIN, OUTPUT);
    pinMode(GREEN_PIN, OUTPUT);
    setState(state, Action::INIT);

    //initializing light sequence
    for (int i = 0; i < 10; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(200);
        digitalWrite(LED_BUILTIN, LOW);
        delay(200);
    }

    Serial.println("Setup complete...");
    printConfiguration(state);
}

// program template for hardware test.  If led still flashing when pump runs then there is no brownout issue

void loop() {
    updateGradient(millis(), SLEEP_MS, state.pumpStopTime, 350);
    button(state);
    if (!state.buttonActivated) intervalMode(state);
    pump(state);
}

void button(SharedState& s) {
    if (isButtonReleased(PIN_BUTTON)) {
        if (s.pumpRunning) {
            s.pumpRunning = false;
            s.buttonActivated = false;
        }
        else {
            // setState(s, Action::WATER);
            s.pumpRunning = true;
            s.buttonActivated = true;
        }
    }
}

void pump(SharedState& s) {
    if (s.pumpRunning) {
        digitalWrite(LED_BUILTIN, HIGH);
        digitalWrite(PIN_PUMP_GATE, HIGH);
    }
    else {
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(PIN_PUMP_GATE, LOW);
    }
}

void intervalMode(SharedState& s) {
    if (!s.pumpRunning) {
        // led gradient here
        if (isElapsedMillis(s.pumpStopTime, SLEEP_MS)) {
            if (s.activations < MAX_ACTIVATIONS) {
                setState(s, Action::WATER);
                Serial.print("Activations: ");
                Serial.println(s.activations);
                Serial.print("Activation limit: ");
                Serial.println(MAX_ACTIVATIONS);
            }
            else {
                // Serial.println("Activation limit reached, refill water and toggle on off to reset.");
                //s.pumpStopTime = millis(); // reset to avoid rapid flashing
                if (isFlashingMillis(ERROR_FLASH_INTERVAL_MS)) {
                    digitalWrite(LED_BUILTIN, HIGH);
                    setRGB(255, 0, 0);
                }
                else {
                    digitalWrite(LED_BUILTIN, LOW);
                    setRGB(0, 0, 0);
                }
            }
        }
    }
    else {
        if (isElapsedMillis(s.pumpStartTime, WATER_MS)) {
            Serial.println("Water interval complete, stopping pump.");
            setState(s, Action::DRY);
        }
    }
}

uint32_t totalActivations(float tankCapacityGallons, float gph, uint32_t waterIntervalMs) {
    if (gph <= 0.0f || waterIntervalMs == 0) return 0;

    const float gallonsPerAct = (gph / 3600000.0f) * waterIntervalMs;
    if (gallonsPerAct <= 0.0f) return 0;

    const float raw = tankCapacityGallons / gallonsPerAct;
    return (raw > 0.0f) ? (uint32_t)floorf(raw) : 0;
}

void setState(SharedState& s, Action a) {
    switch (a) {
    case Action::INIT:
        s.pumpRunning = false;
        s.pumpStopTime = millis();
        setRGB(0, 255, 43);
        break;
    case Action::WATER:
        // water the plant
        if (s.pumpRunning) {
            Serial.println("Pump is already running.");
        }
        else {
            // Serial.println("Starting pump...");
            s.pumpRunning = true;       // set pump running state
            s.pumpStartTime = millis(); // reset pump start time
            s.activations++; // increment activations
        }
        break;
    case Action::DRY:
        if (!s.pumpRunning) {
            Serial.println("Pump is not running.");
        }
        else {
            // Serial.println("Stopping pump...");
            s.pumpRunning = false; // set pump running state
            s.pumpStopTime = millis();
        }
        break;
    default:
        break;
    }
}

void setRGB(uint8_t r, uint8_t g, uint8_t b) {
    if (COMMON_ANODE) { r = 255 - r; g = 255 - g; b = 255 - b; }  // invert for CA
    analogWrite(RED_PIN, r); analogWrite(GREEN_PIN, g); analogWrite(BLUE_PIN, b);
}

void updateGradient(uint32_t now, uint32_t intervalMs, uint32_t lastWaterStop, uint32_t stepMs) {
    static uint32_t lastStep = 0;
    if (now - lastStep < stepMs) return;
    lastStep = now;

    // Compute fraction of time remaining (f in [0..1])
    uint32_t elapsed = now - lastWaterStop;                 // rollover-safe
    if (elapsed >= intervalMs) elapsed = intervalMs;           // clamp
    float f = (intervalMs == 0) ? 0.0f : ((intervalMs - elapsed) / (float)intervalMs);
    // f=1 → start (full time left, blue). f=0 → end (no time left, green).
    // 1 -> 0 over time

    // Linear blend: (R,G,B) = (0, 255*(1-f), 255*f)
    uint8_t r = 0;
    uint8_t g = (uint8_t)(255.0f * (1.0f - f) + 0.5f); //math rount to nearest int
    uint8_t b = (uint8_t)(255.0f * f + 0.5f);

    // Optional gamma
    // g = gamma8(g);  b = gamma8(b);

    setRGB(r, g, b);
}

void printConfiguration(SharedState& s) {
    Serial.println("Configuration:");
    Serial.println(" Mode = INTERVAL ");
    Serial.print("  Water Interval (min): ");
    Serial.println(WATER_MS / 60000.0); //ms to minutes
    Serial.print("  Sleep Time (days): ");
    Serial.println(SLEEP_MS / 86400000.0); //ms to days
    Serial.print("  Tank Capacity (gallons): ");
    Serial.println(TANK_CAPACITY_GALLONS);
    Serial.print("  GPH Total: ");
    Serial.println(GPH_TOTAL);
    Serial.print("  Max Activations: ");
    Serial.println(MAX_ACTIVATIONS);
    Serial.print("  pump state: ");
    Serial.println(s.pumpRunning);
}