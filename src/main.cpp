#include <Arduino.h>
#include "config.h"
#include "MoistureSensor.h"
#ifdef ESP32
#include <TFT_eSPI.h>
#include <esp_sleep.h>
#include "Pwm.h"
TFT_eSPI tft;
#endif

SharedState state;

// internal setup
void setup()
{
    Serial.begin(115200);
    delay(1000); // for hardware stabilization on wakeup

    // Initialize GPIO for LED, button, and pump
    pinMode(LED_BUILTIN, OUTPUT);
    // pinMode(PIN_BUTTON, INPUT_PULLUP);
    // Optional dual buttons for INIT nav
    
    // Joystick setup
    #ifdef ESP32
    pinMode(PIN_BUTTON_RIGHT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_LEFT, INPUT_PULLUP);
    pinMode(PIN_JOY_SW, INPUT_PULLUP);
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_JOY_X, ADC_11db);
    analogSetPinAttenuation(PIN_JOY_Y, ADC_11db);
    // Initialize TFT
       pinMode(TFT_BL, OUTPUT);
       digitalWrite(TFT_BL, HIGH);    // REQUIRED or the screen looks off
    
      
       tft.init();                    // requires correct Setup25
       tft.setRotation(1);
       tft.fillScreen(TFT_BLACK);
       tft.setTextColor(tft.color565(0, 255, 65), TFT_BLACK); // Malachite green
       // Init state
       initPWM(PIN_PUMP_GATE, PWM_CH, PWM_FREQ, PWM_RES_BITS);
       #endif
       pinMode(PIN_LED_LARGE, OUTPUT);
    pinMode(PIN_PUMP_GATE, OUTPUT);
    setState(state, Action::INIT_SETUP);

    // //Init sensor if in monitor mode
    // if (state.mode == Mode::MONITOR)
    // {
    //     initSensor(PIN_SENSOR_PWR, PIN_SOIL_ADC);
    // }

    // INIT PWM

    //print configuration and flash for 10 seconds to show ready
    // no delays after setup, only use timing to avoid blocking and important tasks like button handling
    //printConfiguration(state);
    // for (int i = 0; i < 2; i++) {
    //     digitalWrite(LED_BUILTIN, HIGH);
    //     digitalWrite(PIN_LED_LARGE, HIGH);
    //     delay(1000);
    //     digitalWrite(LED_BUILTIN, LOW);
    //     digitalWrite(PIN_LED_LARGE, LOW);
    //     delay(1000);
    // }
    // for (int i = 0; i < 3; i++) {
    //     digitalWrite(LED_BUILTIN, HIGH);
    //     digitalWrite(PIN_LED_LARGE, HIGH);
    //     delay(500);
    //     digitalWrite(LED_BUILTIN, LOW);
    //     digitalWrite(PIN_LED_LARGE, LOW);
    //     delay(500);
    // }
    // for (int i = 0; i < 4; i++) {
    //     digitalWrite(LED_BUILTIN, HIGH);
    //     digitalWrite(PIN_LED_LARGE, HIGH);
    //     delay(250);
    //     digitalWrite(LED_BUILTIN, LOW);
    //     digitalWrite(PIN_LED_LARGE, LOW);
    //     delay(250);
    // }
}

void loop()
{
    if (state.mode == Mode::INIT) {
        #ifdef ESP32
        initMode(state);
        #else
        initModeLight(state);
        #endif
    }
    else
    {
        if (state.mode == Mode::INTERVAL)
        {
            intervalMode(state);
        }
        else if (state.mode == Mode::MONITOR)
        {
            //monitorMode(state);
        }
        button(state);
        pump(state);
        #ifdef ESP32
        displayConfiguration(state);
        #else 
        printConfiguration(state);
        #endif
    }
}