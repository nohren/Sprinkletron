#pragma once

// ---- Pin map (change to match your wiring)
#define PIN_SOIL_ADC        36    // sensor analog out -> ESP32 GPIO36
#define PIN_SENSOR_PWR      19    // drives a transistor/high-side switch to power the sensor
#define PIN_PUMP_GATE       18    // goes to MOSFET gate that switches the pump
#define PIN_BUTTON          23     // push button to toggle pump on/off

// Many WROOM-32 dev boards wire the blue LED to GPIO 2.
// If LED_BUILTIN isn't defined, fall back to 2.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

static const uint8_t PWM_CH       = 0;      // 0..15
static const uint32_t PWM_FREQ     = 20000;  // 20 kHz (above audible range)
static const uint8_t PWM_RES_BITS  = 10;     // 10-bit (0..1023)
static const uint32_t SLEEP_HOURS  = 72;     // deep sleep interval in hours
static const uint8_t MIN_RUN_PCT = 55;
static const unsigned long FLASH_INTERVAL_MS = 1000; // for led
static const unsigned long MAX_PUMP_TIME_MS = 15000; // maximum time to run the pump in milliseconds
static const unsigned long SOFTSTART_MS = 300;  // 0.3s kick

enum class Action {
    DRY,       // sense dry or wet soil
    WATER,      // water the plant
    SLEEP       // go to deep sleep
};

//state 
struct SharedState {
    bool pumpRunning = false;   // true if pump is currently running
    unsigned long lastSampleTime = 0; // last time we sampled the sensor
    unsigned long pumpStartTime = 0;   // last time we ran the pump
    unsigned long pumpStopTime = 0;    // last time we stopped the pump
    int targetPumpPct = 80; // target pump duty cycle in percent (0-100)
};

// ---- Function declarations
void setState(SharedState &s, Action a);
void pump(SharedState &s); // control pump based on state
void button(SharedState &s);
void sampleSoil(SharedState &s, bool verbose);
void setPumpPct(int pct); // set pump duty cycle (0-100%)
void drivePump(bool on, SharedState &s);
float getVoltage(int raw);
float getAverageVoltage(float readings[], int size);
void safeSleep(uint32_t hours);
void safeSleepSeconds(uint32_t seconds); // for testing purposes, sleeps for a number of seconds
