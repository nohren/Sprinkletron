#pragma once

// ---- Pin map (change to match your wiring)
#define PIN_SOIL_ADC        36    // sensor analog out -> ESP32 GPIO36
#define PIN_SENSOR_PWR      19    // drives a transistor/high-side switch to power the sensor
#define PIN_PUMP_GATE       18    // goes to MOSFET gate that switches the pump

// ---- Behavior
#define WAKE_INTERVAL_HOURS     24                     // deep sleep interval
#define PUMP_MS                 6000                  // run pump this long if dry (failsafe upper bound)
#define COOLDOWN_AFTER_PUMP_MS  2000                  // brief settle after watering (optional)

// Sensor warmup + sampling
#define SENSOR_POWER_SETTLE_MS  200                   // allow sensor to stabilize after power-on
#define SENSOR_SAMPLES          32                    // take N samples and use median

// ---- Moisture calibration
// Read raw ADC in AIR (bone dry) and in WATER (fully wet) once and fill these:
#define RAW_AIR      3200   // example placeholder, measure yours!
#define RAW_WATER    1400   // example placeholder, measure yours!

// Threshold in % moisture below which we water
#define MOISTURE_THRESHOLD_PCT  35

// ---- Safety: minimum hours between water events (avoid stuck-on sensor causing floods)
#define MIN_HOURS_BETWEEN_WATER  12
