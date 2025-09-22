#pragma once
// #include "Intervals.h"

// ---- Configuration ---------   Change these to match your setup and preferences

// ---- Pin map
#define PIN_SOIL_ADC 32   // sensor analog out -> ESP32 GPIO36
#define PIN_SENSOR_PWR 38 // drives a transistor/high-side switch to power the sensor
#define PIN_PUMP_GATE 21  // goes to MOSFET gate that switches the pump
#define PIN_BUTTON 15     // push button to toggle pump on/off
#define PIN_LED_LARGE 12
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#define TFT_BL 4  

// interval mode settings
static const float GPH_TOTAL = 4; // total gallon per hour heads on the line if using rainbird GPH heads
static const float TANK_CAPACITY_GALLONS = 2.905; //11 liter tank has 2.905 gallons
static const uint16_t WATER_MINUTES = 2;
static const uint16_t SLEEP_DAYS = 5;

// monitor mode settings
static const float MIN_MOISTURE_THRESHOLD = 2.5;     // // pretty dry... this is an empirically derived sensor voltage value
static const float MAX_MOISTURE_THRESHOLD = 1.8;     // pretty damp ... same as above
static const uint32_t MAX_PUMP_TIME_SECONDS = 15; // maximum time to run the pump in seconds in monitor mode... this is a failsafe for faulty sensor
static const uint16_t SLEEP_HOURS = 24;

// LED interval
static const uint32_t FLASH_INTERVAL_MS = 1000; // for led

//PWM settings for ESP32
static const uint8_t PWM_CH = 0;        // 0..15
static const uint32_t PWM_FREQ = 20000; // 20 kHz (above audible range)
static const uint8_t PWM_RES_BITS = 10; // 10-bit (0..1023)
static const uint8_t MIN_RUN_PCT = 55;
static const uint32_t SOFTSTART_MS = 300;       // 0.3s kick

// --------  USED INTERNALLY - DO NOT CHANGE -------------
static const uint64_t SLEEP_US = 0;//hoursToMicros(SLEEP_HOURS);
static const uint32_t WATER_MS = 0;//minutesToMillis(WATER_MINUTES);
static const uint32_t MAX_PUMP_TIME_MS = 0;//secondsToMillis(MAX_PUMP_TIME_SECONDS);

enum class Action
{
    INIT_SETUP, // init state
    INIT_INTERVAL,
    INIT_MONITOR,
    DRY,   // sense dry or wet soil
    WATER, // water the plant
    SLEEP,  // go to deep sleep
};

enum class Mode
{
    INIT,     // uninitialized
    INTERVAL, // water at regular intervals
    MONITOR   // monitor soil moisture
};

// all times in milliseconds unless otherwise noted
struct SharedState
{
    bool isReady = true; // ready to water for interval mode
    bool pumpRunning = false;   // true if pump is currently running
    uint32_t lastSampleTime = 0; // last time we sampled the sensor
    uint32_t pumpStartTime = 0;  // last time we ran the pump
    uint32_t pumpStopTime = 0;   // last time we stopped the pump
    int targetPumpPct = 100;           // target pump duty cycle in percent (0-100)... can change at runtime
    float tankCapacityGallons = 0;
    uint32_t sleepTime = 0;
    uint32_t waterTime = 0;
    float GPHTotal = 0;
    uint32_t maxActivations = 0; // maximum number of activations before water runs out
    uint32_t activations = 0;
    Mode mode = Mode::INIT; // default mode
};

// ---- Function declarations
void setState(SharedState& s, Action a);
void intervalMode(SharedState& s);
void monitorMode(SharedState& s);
void pump(SharedState& s); // control pump based on state
void button(SharedState& s);
void sampleSoil(SharedState& s, bool verbose);
void safeSleep();
void safeSleepSeconds(uint32_t seconds); // for testing purposes, sleeps for a number of seconds
uint32_t totalActivations(float tankCapacityGallons, float gph, uint32_t waterIntervalMs);
void setState(SharedState& s, Action a);
void printConfiguration(SharedState& s);
