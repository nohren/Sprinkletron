#pragma once
#include <stdint.h>

struct SharedState; // forward declaration

void initSensor(int pinSensor, int pinADC);
float getVoltage(int raw);
float getAverageVoltage(float readings[], int size);
void sampleSoil(SharedState& s, bool verbose = false);