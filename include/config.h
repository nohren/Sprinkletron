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
#define MAX_PUMP_TIME_MS 7000 // maximum time to run the pump in milliseconds (

//state 
struct SharedState {
    bool sensorPowered = false; // true if sensor power is on
    bool pumpRunning = false;   // true if pump is currently running
    unsigned long lastSampleTime = 0; // last time we sampled the sensor
    unsigned long pumpStartTime = 0;   // last time we ran the pump
    bool buttonPressed = false; // for checking when its false when previous was true to turn on off pump.

};

// ---- Function declarations
void pump(SharedState &s); // control pump based on state
void button(SharedState &s);
void sampleSoil(SharedState &s, bool verbose);
float getVoltage(int raw);

// // ---- Behavior
// #define WAKE_INTERVAL_HOURS     24                     // deep sleep interval
// #define PUMP_MS                 6000                  // run pump this long if dry (failsafe upper bound)
// #define COOLDOWN_AFTER_PUMP_MS  2000                  // brief settle after watering (optional)

// // Sensor warmup + sampling
// #define SENSOR_POWER_SETTLE_MS  200                   // allow sensor to stabilize after power-on
// #define SENSOR_SAMPLES          32                    // take N samples and use median

// // ---- Moisture calibration
// // Read raw ADC in AIR (bone dry) and in WATER (fully wet) once and fill these:
// #define RAW_AIR      3200   // example placeholder, measure yours!
// #define RAW_WATER    1400   // example placeholder, measure yours!

// // Threshold in % moisture below which we water
// #define MOISTURE_THRESHOLD_PCT  35

// // ---- Safety: minimum hours between water events (avoid stuck-on sensor causing floods)
// #define MIN_HOURS_BETWEEN_WATER  12
