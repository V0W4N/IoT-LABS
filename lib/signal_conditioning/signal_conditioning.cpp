#include <Arduino.h>

#include "signal_conditioning.h"

namespace {

void insertionSort(float *data, size_t count) {
    for (size_t i = 1; i < count; ++i) {
        float key = data[i];
        size_t j = i;
        while (j > 0 && data[j - 1] > key) {
            data[j] = data[j - 1];
            --j;
        }
        data[j] = key;
    }
}

}  // namespace

namespace signal_conditioning {

// -----------------------------------------------------------------------------
// Salt & Pepper (Median) Filter
// -----------------------------------------------------------------------------

void saltPepperInit(SaltPepperFilter &filter, size_t windowSize) {
    if (windowSize < 3) {
        windowSize = 3;
    }
    if (windowSize > SALT_PEPPER_MAX_WINDOW) {
        windowSize = SALT_PEPPER_MAX_WINDOW;
    }
    if (windowSize % 2 == 0) {
        windowSize += 1;  // ensure odd window size
        if (windowSize > SALT_PEPPER_MAX_WINDOW) {
            windowSize = SALT_PEPPER_MAX_WINDOW;
        }
    }

    filter.windowSize = windowSize;
    filter.count = 0;
    filter.index = 0;
    for (size_t i = 0; i < SALT_PEPPER_MAX_WINDOW; ++i) {
        filter.values[i] = 0.0f;
    }
}

float saltPepperProcess(SaltPepperFilter &filter, float sample) {
    filter.values[filter.index] = sample;
    filter.index = (filter.index + 1) % filter.windowSize;
    if (filter.count < filter.windowSize) {
        filter.count++;
    }

    float window[SALT_PEPPER_MAX_WINDOW];
    for (size_t i = 0; i < filter.count; ++i) {
        window[i] = filter.values[i];
    }

    insertionSort(window, filter.count);
    size_t medianIndex = filter.count / 2;
    return window[medianIndex];
}

// -----------------------------------------------------------------------------
// Weighted Moving Average Filter
// -----------------------------------------------------------------------------

void weightedMovingAverageInit(WeightedMovingAverage &filter,
                               const float *weights,
                               size_t tapCount) {
    if (tapCount < 1) {
        tapCount = 1;
    }
    if (tapCount > WMA_MAX_TAPS) {
        tapCount = WMA_MAX_TAPS;
    }

    filter.tapCount = tapCount;
    filter.index = 0;
    filter.count = 0;
    filter.weightSum = 0.0f;

    for (size_t i = 0; i < WMA_MAX_TAPS; ++i) {
        filter.buffer[i] = 0.0f;
    }

    for (size_t i = 0; i < tapCount; ++i) {
        filter.weights[i] = weights[i];
        filter.weightSum += weights[i];
    }

    if (filter.weightSum == 0.0f) {
        filter.weightSum = 1.0f;
    }
}

float weightedMovingAverageProcess(WeightedMovingAverage &filter, float sample) {
    filter.buffer[filter.index] = sample;
    filter.index = (filter.index + 1) % filter.tapCount;
    if (filter.count < filter.tapCount) {
        filter.count++;
    }

    float accumulator = 0.0f;
    float weightsTotal = 0.0f;

    for (size_t i = 0; i < filter.count; ++i) {
        size_t bufferIndex = (filter.index + filter.tapCount - 1 - i) % filter.tapCount;
        float weight = filter.weights[i];
        accumulator += filter.buffer[bufferIndex] * weight;
        weightsTotal += weight;
    }

    if (weightsTotal == 0.0f) {
        return accumulator;
    }

    return accumulator / weightsTotal;
}

// -----------------------------------------------------------------------------
// Utility helpers
// -----------------------------------------------------------------------------

float applySaturation(float value, float minValue, float maxValue) {
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return value;
}

float adcToVoltage(uint16_t adcValue, float referenceVoltage, uint16_t adcResolution) {
    if (adcResolution == 0) {
        return 0.0f;
    }
    return (static_cast<float>(adcValue) * referenceVoltage) / static_cast<float>(adcResolution);
}

float voltageToAdc(float voltage, float referenceVoltage, uint16_t adcResolution) {
    if (referenceVoltage == 0.0f) {
        return 0.0f;
    }
    float normalized = voltage / referenceVoltage;
    normalized = applySaturation(normalized, 0.0f, 1.0f);
    return normalized * static_cast<float>(adcResolution);
}

}  // namespace signal_conditioning


