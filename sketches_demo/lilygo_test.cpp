#include <Arduino.h>
#include <TFT_eSPI.h>

#define TFT_BL 4             // classic T-Display backlight pin

TFT_eSPI tft;

void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);    // REQUIRED or the screen looks off

  tft.init();                    // requires correct Setup25
  tft.setRotation(1);

  // Quick color test (helps spot misconfig)
  tft.fillScreen(TFT_RED);   delay(250);
  tft.fillScreen(TFT_GREEN); delay(250);
  tft.fillScreen(TFT_BLUE);  delay(250);

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Hello, T-Display!", 20, 20, 2);
}

void loop() {
  // Tiny pause keeps the task watchdog happy even on heavy work
  delay(1);
}
