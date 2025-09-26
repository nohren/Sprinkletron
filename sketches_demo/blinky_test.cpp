#include <Arduino.h>

// Many WROOM-32 dev boards wire the blue LED to GPIO 2.
// If LED_BUILTIN isn't defined, fall back to 2.
// If yours doesn’t blink, change to 5 and try again.
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Some boards wire the LED active-LOW. Flip this if your
// LED seems “on” when you write LOW.
#ifndef LED_ACTIVE_LOW
#define LED_ACTIVE_LOW 0
#endif

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Initialize the LED pin as an output i.e driving out signal / current to control the LED.
}

//low level programming is loopy
void loop() {
  // turn LED on
  digitalWrite(LED_BUILTIN, LED_ACTIVE_LOW ? LOW : HIGH);
  delay(500);
  // turn LED off
  digitalWrite(LED_BUILTIN, LED_ACTIVE_LOW ? HIGH : LOW);
  delay(500);
}