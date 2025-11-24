#ifndef THERMISTOR_UTILS_H
#define THERMISTOR_UTILS_H

#include <Arduino.h>

struct ThermistorConfig {
    float beta;             // Beta coefficient
    float r0;               // Resistance at reference temperature (Ohms)
    float t0Kelvin;         // Reference temperature (Kelvin)
    float seriesResistor;   // Series resistor value (Ohms)
    float vRef;             // ADC reference voltage (Volts)
    uint16_t adcResolution; // ADC resolution (e.g., 1023)
};

struct ThermistorSample {
    uint16_t adcValue;
    float voltage;
    float resistance;
    float temperatureC;
};

struct ThermistorStream {
    ThermistorConfig config;
    uint8_t analogPin;
    uint8_t oversampleCount;
    ThermistorSample lastSample;
    char buffer[32];
    size_t bufferIndex;
    size_t bufferLength;
};

void thermistorConfigInit(ThermistorConfig &config,
                          float beta,
                          float nominalResistance,
                          float nominalTempC,
                          float seriesResistor,
                          float referenceVoltage,
                          uint16_t adcResolution);

float thermistorVoltageToResistance(const ThermistorConfig &config, float voltage);
float thermistorResistanceToTemperatureC(const ThermistorConfig &config, float resistance);
float thermistorAdcToTemperatureC(const ThermistorConfig &config, uint16_t adcValue);

void thermistorStreamInit(ThermistorStream &stream,
                          const ThermistorConfig &config,
                          uint8_t analogPin,
                          uint8_t oversampleCount);

int thermistorStreamGetchar(ThermistorStream &stream);

const ThermistorSample &thermistorStreamGetLastSample(const ThermistorStream &stream);

#endif  // THERMISTOR_UTILS_H
