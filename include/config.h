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

// CONFIGURATION
#define MAX_PUMP_TIME_MS 15000 // maximum time to run the pump in milliseconds (
#define SLEEP_HOURS 72 // deep sleep interval in hours
#define LOW_MOISTURE_THRESHOLD_VOLTAGE 2.4
#define HIGH_MOISTURE_THRESHOLD_VOLTAGE 1.7
#define SOFTSTART_MS 1000  // 0.3s kick
#define MIN_RUN_PCT 55

//state 
struct SharedState {
    bool sensorPowered = false; // true if sensor power is on
    bool pumpRunning = false;   // true if pump is currently running
    unsigned long lastSampleTime = 0; // last time we sampled the sensor
    unsigned long pumpStartTime = 0;   // last time we ran the pump
    bool buttonPressed = false; // for checking when its false when previous was true to turn on off pump.
    int targetPumpPct = 65; // target pump duty cycle in percent (0-100)
};

// ---- Function declarations
void pump(SharedState &s); // control pump based on state
void button(SharedState &s);
void sampleSoil(SharedState &s, bool verbose);
void setPumpPct(int pct); // set pump duty cycle (0-100%)
void drivePump(bool on, SharedState &s);
float getVoltage(int raw);

// // ---- Behavior

// #define COOLDOWN_AFTER_PUMP_MS  2000                  // brief settle after watering (optional)

// // Sensor warmup + sampling
// #define SENSOR_POWER_SETTLE_MS  200                   // allow sensor to stabilize after power-on
// #define SENSOR_SAMPLES          32                    // take N samples and use median

// // ---- Moisture calibration
// // Read raw ADC in AIR (bone dry) and in WATER (fully wet) once and fill these:
// #define RAW_AIR      3200   // example placeholder, measure yours!
// #define RAW_WATER    1400   // example placeholder, measure yours!

// // Threshold in % moisture below which we water


// // ---- Safety: minimum hours between water events (avoid stuck-on sensor causing floods)
// #define MIN_HOURS_BETWEEN_WATER  12
