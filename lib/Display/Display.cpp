// #ifdef ESP32
#include <TFT_eSPI.h> // if error, it means this library is not inluded in platformio.ini. add ; lib_deps =
//                  ;   Bodmer/TFT_eSPI
void init(int pin) {
    // define and pass in TFT_BL as pin in program main
    pinMode(pin, OUTPUT);
    digitalWrite(pin, HIGH);    // REQUIRED or the screen looks off

    TFT_eSPI tft = TFT_eSPI();      // Invoke custom library
    tft.init();                    // requires correct Setup25
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(tft.color565(0, 255, 65), TFT_BLACK); // matrix green
}



//#endif