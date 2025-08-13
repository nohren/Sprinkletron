#include <Arduino.h>
#include "config.h"

SharedState state; // global state

/*
  C++ pass by reference example, i.e we want the function to modify the state of the original struct
  void update(AppState &s) {  
    s.lastSampleTime = millis(); // changes the original
}
*/

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_PUMP_GATE, OUTPUT);
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
        if (voltage > 2.4) {
            if (!s.pumpRunning) {
                Serial.println("Soil is dry, starting pump...");
                s.pumpRunning = true; // if voltage is high, we assume soil is dry
                s.pumpStartTime = millis(); // update last pump time
            }
        } else if (voltage < 1.7) {
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
        s.pumpRunning = true;
        s.pumpStartTime = millis(); // doing this to override the pump timer on button press
    } else {
        s.pumpRunning = false;
    }
}

void pump(SharedState &s) {
    if (s.pumpRunning) {
        // failsafe trip: if pump has been running too long, sensor might be broken, stop it
        if (millis() - s.pumpStartTime > MAX_PUMP_TIME_MS) {
            s.pumpRunning = false; // stop pump after 10 seconds
        }
        digitalWrite(LED_BUILTIN, HIGH); // turn on LED
        digitalWrite(PIN_PUMP_GATE, HIGH); // turn on pump  
    } else {
        digitalWrite(LED_BUILTIN, LOW); // turn off LED
        digitalWrite(PIN_PUMP_GATE, LOW); // turn off pump
    }
}