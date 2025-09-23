#include <Arduino.h>
#include "config.h"
#include "MoistureSensor.h"
#include "Intervals.h"

void initSensor(int pinSensor, int pinADC)
{
    //sensor power
    pinMode(pinSensor, OUTPUT);
    digitalWrite(pinSensor, HIGH);
#ifdef ESP32
    // ADC setup
    analogReadResolution(12);                        // ESP32 supports 9–12 bits (default is 12)
    analogSetPinAttenuation(pinADC, ADC_11db); // ADC_11db lets you read up to about 3.6V input
#endif
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

// placeholder implementation to satisfy linker; logic can be re-enabled later
void sampleSoil(SharedState& s, bool verbose)
{
   return;
    // minimal stub: transition immediately to sleep until full logic is restored
   

    // if ((millis() - s.lastSampleTime) > 100)
    // {
    //     int rawValue = analogRead(PIN_SOIL_ADC); // 0–4095 at 12-bit
    //     float voltage = getVoltage(rawValue);

    //     s.lastSampleTime = millis();       // update last sample time
    //     lastReadings[readIndex] = voltage; // store the reading
    //     readIndex++;
    //     if (readIndex >= SAMPLE_COUNT)
    //     {
    //         readIndex = 0; // wrap around
    //         // calculate average
    //         float averageVoltage = getAverageVoltage(lastReadings, SAMPLE_COUNT);
    //         if (verbose)
    //         {
    //             // Serial.print("Soil ADC Raw: ");
    //             // Serial.print(rawValue);
    //             Serial.print("   Voltage: ");
    //             Serial.print(averageVoltage, 3);
    //             Serial.println(" V");
    //         }

    //         // Decision making based on 30 average voltage readings
    //         // usually like 30 seconds of monitoring
    //         if (monitorMode)
    //         {
    //             decisionCount--;
    //             if (decisionCount == 0)
    //             {
    //                 monitorMode = false;
    //                 if (averageVoltage > MIN_MOISTURE_THRESHOLD)
    //                 {
    //                     setState(s, Action::WATER);
    //                 }
    //                 else
    //                 {
    //                     Serial.println("Soil moisture is within acceptable range.");
    //                     // go back to sleep
    //                     setState(s, Action::SLEEP);
    //                 }
    //             }
    //         }
    //         else
    //         {
    //             // if we are here, it means we are in water mode
    //             // stop watering when too moist, go to sleep
    //             if ((averageVoltage < MAX_MOISTURE_THRESHOLD) || ((millis() - s.pumpStartTime) > MAX_PUMP_TIME_MS))
    //             {
    //                 Serial.println("Soil moisture is too high, stopping watering.");
    //                 setState(s, Action::SLEEP);
    //             }
    //         }
    //     }
    // }
}