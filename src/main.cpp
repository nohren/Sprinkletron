#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
// #include "MoistureSensor.h"
// #ifdef ESP32
// #include <esp_sleep.h>
// #include "Pwm.h"
// #endif

// SharedState state;
TFT_eSPI tft;

// internal setup
void setup()
{
    Serial.begin(115200);
    delay(1000); // for hardware stabilization on wakeup

    // Initialize GPIO for LED, button, and pump
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    pinMode(PIN_PUMP_GATE, OUTPUT);
    pinMode(PIN_LED_LARGE, OUTPUT);

    // Initialize TFT
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);    // REQUIRED or the screen looks off

    tft.init();                    // requires correct Setup25
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Mode: INIT", 20, 20, 2);
    // // Init state
    // setState(state, Action::INIT_SETUP);

    // Init sensor if in monitor mode
    // if (state.mode == Mode::MONITOR)
    // {
    //     initSensor(PIN_SENSOR_PWR, PIN_SOIL_ADC);
    // }

    // INIT PWM
#ifdef ESP32
// set ADC resolution and attenuation
    //initPWM(PIN_PUMP_GATE, PWM_CH, PWM_FREQ, PWM_RES_BITS);
#endif
    //print configuration and flash for 10 seconds to show ready
    // no delays after setup, only use timing to avoid blocking and important tasks like button handling
    //printConfiguration(state);
    // for (int i = 0; i < 5; i++) {
    //     digitalWrite(LED_BUILTIN, HIGH);
    //     digitalWrite(PIN_LED_LARGE, HIGH);
    //     delay(1000);
    //     digitalWrite(LED_BUILTIN, LOW);
    //     digitalWrite(PIN_LED_LARGE, LOW);
    //     delay(1000);
    // }
}

void loop()
{
    // if (state.mode == Mode::INIT) {
    //     // replace with user inputs values here
    //     // state.tankCapacityGallons = TANK_CAPACITY_GALLONS;
    //     // state.GPHTotal = GPH_TOTAL;
    //     // state.waterTime = minutesToMillis(WATER_MINUTES);
    //     if (state.tankCapacityGallons > 0 && state.GPHTotal > 0 && state.waterTime > 0) {
    //         setState(state, Action::INIT_INTERVAL);
    //     }
    //     Serial.println("Mode: INIT");
    //     tft.drawString("Mode: INIT", 20, 20, 2);
    // }
    // else
    // {
    //     if (state.mode == Mode::INTERVAL)
    //     {
    //         intervalMode(state);
    //     }
    //     else if (state.mode == Mode::MONITOR)
    //     {
    //         monitorMode(state);
    //     }
    //     button(state);
    //     pump(state);
    // }
}