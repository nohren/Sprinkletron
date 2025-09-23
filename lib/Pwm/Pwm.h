// PWM for ESP32 
#pragma once

#ifdef ESP32
void initPWM(uint8_t pin, uint8_t PWM_CH, uint32_t PWM_FREQ, uint8_t PWM_RES_BITS);
void drivePump(bool on, int targetPumpPct, int MIN_RUN_PCT, uint32_t pumpStartTime, uint32_t SOFTSTART_MS, uint8_t PWM_CH, uint8_t PWM_RES_BITS);
void setPumpPct(int pct, uint8_t PWM_CH, uint8_t PWM_RES_BITS); // set pump duty cycle (0-100%)
#endif