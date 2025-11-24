#ifndef SIGNAL_CONDITIONING_H
#define SIGNAL_CONDITIONING_H

#include <Arduino.h>

namespace signal_conditioning {

// -----------------------------------------------------------------------------
// Salt & Pepper (Median) Filter
// -----------------------------------------------------------------------------

constexpr size_t SALT_PEPPER_MAX_WINDOW = 9;

struct SaltPepperFilter {
    size_t windowSize;
    float values[SALT_PEPPER_MAX_WINDOW];
    size_t count;
    size_t index;
};

void saltPepperInit(SaltPepperFilter &filter, size_t windowSize);
float saltPepperProcess(SaltPepperFilter &filter, float sample);

// -----------------------------------------------------------------------------
// Weighted Moving Average Filter
// -----------------------------------------------------------------------------

constexpr size_t WMA_MAX_TAPS = 8;

struct WeightedMovingAverage {
    size_t tapCount;
    float weights[WMA_MAX_TAPS];
    float buffer[WMA_MAX_TAPS];
    size_t index;
    size_t count;
    float weightSum;
};

void weightedMovingAverageInit(WeightedMovingAverage &filter,
                               const float *weights,
                               size_t tapCount);
float weightedMovingAverageProcess(WeightedMovingAverage &filter, float sample);

// -----------------------------------------------------------------------------
// Utility helpers
// -----------------------------------------------------------------------------

float applySaturation(float value, float minValue, float maxValue);
float adcToVoltage(uint16_t adcValue, float referenceVoltage, uint16_t adcResolution);
float voltageToAdc(float voltage, float referenceVoltage, uint16_t adcResolution);

}  // namespace signal_conditioning

#endif  // SIGNAL_CONDITIONING_H


