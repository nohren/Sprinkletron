// PWM for ESP32
#include <Arduino.h>
#ifdef ESP32
void initPWM(uint8_t pin, uint8_t PWM_CH, uint32_t PWM_FREQ, uint8_t PWM_RES_BITS)
{
    ledcSetup(PWM_CH, PWM_FREQ, PWM_RES_BITS);
    ledcAttachPin(pin, PWM_CH);
    ledcWrite(PWM_CH, 0); // start off
}

void setPumpPct(int pct, uint8_t PWM_CH, uint8_t PWM_RES_BITS)
{
    if (pct < 0)
        pct = 0;
    if (pct > 100)
        pct = 100;
    const int maxDuty = (1 << PWM_RES_BITS) - 1; // e.g., 1023
    int duty = (pct * maxDuty) / 100;
    ledcWrite(PWM_CH, duty);
}

void drivePump(bool on, int targetPumpPct, int MIN_RUN_PCT, uint32_t pumpStartTime, uint32_t SOFTSTART_MS, uint8_t PWM_CH, uint8_t PWM_RES_BITS)
{
    if (!on)
    {
        setPumpPct(0, PWM_CH, PWM_RES_BITS);
        return;
    }

    // clamp to a safe minimum to avoid stall
    int pct = targetPumpPct < MIN_RUN_PCT ? MIN_RUN_PCT : targetPumpPct;

    if (millis() - pumpStartTime < SOFTSTART_MS)
    {
        setPumpPct(100, PWM_CH, PWM_RES_BITS); // kick
    }
    else
    {
        setPumpPct(pct, PWM_CH, PWM_RES_BITS);
    }
}
#endif