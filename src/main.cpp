#include <Arduino.h>
#include "config.h"

SharedState state; // global state

static const int PWM_CH       = 0;      // 0..15
static const int PWM_FREQ     = 20000;  // 20 kHz (above audible range)
static const int PWM_RES_BITS = 10;     // 10-bit (0..1023)

void setup() {
    Serial.begin(115200);
    delay(1000);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_PUMP_GATE, OUTPUT);
    ledcSetup(PWM_CH, PWM_FREQ, PWM_RES_BITS);
    ledcAttachPin(PIN_PUMP_GATE, PWM_CH);
    // optional: start off
    ledcWrite(PWM_CH, 0);

    // Optional: set ADC resolution and attenuation
    analogReadResolution(12); // ESP32 supports 9–12 bits (default is 12)
    analogSetPinAttenuation(PIN_SOIL_ADC, ADC_11db);  // ADC_11db lets you read up to about 3.6V input
    state.lastSampleTime = millis(); // initialize last sample time
}

// adding multiple sensors and functionality, no delay's
// use millis() for timing
void loop() {
    sampleSoil(state, true);
    pump(state); 
    button(state);
}

void sampleSoil(SharedState &s, bool verbose = false) {
    if (millis() - s.lastSampleTime > 1000) {
        int rawValue = analogRead(PIN_SOIL_ADC); // 0–4095 at 12-bit
        float voltage = getVoltage(rawValue);
        if (verbose) {
            Serial.print("Soil ADC Raw: ");
            Serial.print(rawValue);
            Serial.print("   Voltage: ");
            Serial.print(voltage, 3);
            Serial.println(" V");
        }
        // We run until soil becomes too wet or timer expires
        if (voltage > LOW_MOISTURE_THRESHOLD_VOLTAGE) {
            if (!s.pumpRunning) {
                Serial.println("Soil is dry, starting pump...");
                s.pumpRunning = true; // if voltage is high, we assume soil is dry
                s.pumpStartTime = millis(); // update last pump time
            }
        } else if (voltage < HIGH_MOISTURE_THRESHOLD_VOLTAGE) {
            s.pumpRunning = false; // if voltage is low, we assume soil is wet
        }
        s.lastSampleTime = millis(); // update last sample time
    }
}

// Convert ADC raw value to voltage
float getVoltage(int raw) {
    return (raw / 4095.0) * 3.3; 
}

void button(SharedState &s) {
    if (digitalRead(PIN_BUTTON) == LOW) {
        if (!s.buttonPressed) {
            s.buttonPressed = true; // button was pressed
            s.pumpStartTime = millis(); // reset pump start time
        }
        s.pumpRunning = true;
        //s.pumpStartTime = millis(); // doing this to override the pump timer on button press
        //s.pumpStartTime = millis();
    } else {
        s.pumpRunning = false;
        s.buttonPressed = false; // button is not pressed
    }
}

void pump(SharedState &s) {
    if (s.pumpRunning) {
        // If pump is already running, we can check if we need to stop it
        
        if (millis() - s.pumpStartTime > 1000) {
            s.pumpRunning = false; // stop pump after max seconds
        }
        digitalWrite(LED_BUILTIN, HIGH); // turn on LED
        drivePump(true, s);
    } else {
        digitalWrite(LED_BUILTIN, LOW); // turn off LED
        drivePump(false, s);
    }
}

// Set pump duty cycle (0-100%)
void setPumpPct(int pct) {
  if (pct < 0) pct = 0;
  if (pct > 100) pct = 100;
  const int maxDuty = (1 << PWM_RES_BITS) - 1;  // e.g., 1023
  int duty = (pct * maxDuty) / 100;
  ledcWrite(PWM_CH, duty);
}

void drivePump(bool on, SharedState &s) {
    if (!on) { setPumpPct(0); return; }

    // clamp to a safe minimum to avoid stall
    int pct = s.targetPumpPct < MIN_RUN_PCT ? MIN_RUN_PCT : s.targetPumpPct;
    
    setPumpPct(pct);  
    // if (millis() - s.pumpStartTime < SOFTSTART_MS) {
    //     setPumpPct(100);   // kick
    // } else {
    // }
}