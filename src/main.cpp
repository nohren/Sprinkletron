#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"
#include "MoistureSensor.h"
#ifdef ESP32
#include <esp_sleep.h>
#include "Pwm.h"
#endif

SharedState state;
TFT_eSPI tft;

// internal setup
void setup()
{
    Serial.begin(115200);
    delay(1000); // for hardware stabilization on wakeup

    // Initialize GPIO for LED, button, and pump
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(PIN_BUTTON, INPUT_PULLUP);
    // Optional dual buttons for INIT nav
    pinMode(PIN_BUTTON_LEFT, INPUT_PULLUP);
    pinMode(PIN_BUTTON_RIGHT, INPUT_PULLUP);

    // Joystick setup
    pinMode(PIN_JOY_SW, INPUT_PULLUP);
#ifdef ESP32
    analogReadResolution(12);
    analogSetPinAttenuation(PIN_JOY_X, ADC_11db);
    analogSetPinAttenuation(PIN_JOY_Y, ADC_11db);
#endif
    pinMode(PIN_PUMP_GATE, OUTPUT);
    pinMode(PIN_LED_LARGE, OUTPUT);

    // Initialize TFT
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);    // REQUIRED or the screen looks off

    tft.init();                    // requires correct Setup25
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(tft.color565(0, 255, 65), TFT_BLACK); // Malachite green
    // Init state
    setState(state, Action::INIT_SETUP);

    //Init sensor if in monitor mode
    if (state.mode == Mode::MONITOR)
    {
        initSensor(PIN_SENSOR_PWR, PIN_SOIL_ADC);
    }

    // INIT PWM
#ifdef ESP32
// set ADC resolution and attenuation
    initPWM(PIN_PUMP_GATE, PWM_CH, PWM_FREQ, PWM_RES_BITS);
#endif
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
        initMode(state);
    }
    else
    {
        if (state.mode == Mode::INTERVAL)
        {
            intervalMode(state);
        }
        else if (state.mode == Mode::MONITOR)
        {
            monitorMode(state);
        }
        button(state);
        pump(state);
        displayConfiguration(state);
    }
}