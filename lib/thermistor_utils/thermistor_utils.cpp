#include "thermistor_utils.h"

#include <Arduino.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "signal_conditioning.h"

namespace {

void refreshStreamBuffer(ThermistorStream &stream) {
    uint32_t accumulator = 0;
    uint8_t samples = stream.oversampleCount == 0 ? 1 : stream.oversampleCount;

    for (uint8_t i = 0; i < samples; ++i) {
        accumulator += analogRead(stream.analogPin);
    }

    uint16_t averagedAdc = static_cast<uint16_t>(accumulator / samples);
    float voltage = signal_conditioning::adcToVoltage(averagedAdc, stream.config.vRef, stream.config.adcResolution);
    float resistance = thermistorVoltageToResistance(stream.config, voltage);
    float temperature = thermistorResistanceToTemperatureC(stream.config, resistance);

    stream.lastSample.adcValue = averagedAdc;
    stream.lastSample.voltage = voltage;
    stream.lastSample.resistance = resistance;
    stream.lastSample.temperatureC = temperature;

    int written = snprintf(stream.buffer,
                           sizeof(stream.buffer),
                           "%u\n",
                           static_cast<unsigned int>(averagedAdc));
    if (written < 0) {
        stream.buffer[0] = '\n';
        stream.bufferLength = 1;
    } else {
        size_t length = static_cast<size_t>(written);
        if (length >= sizeof(stream.buffer)) {
            length = sizeof(stream.buffer) - 1;
            stream.buffer[length] = '\n';
        }
        stream.bufferLength = length;
    }
    stream.bufferIndex = 0;
}

}  // namespace

void thermistorConfigInit(ThermistorConfig &config,
                          float beta,
                          float nominalResistance,
                          float nominalTempC,
                          float seriesResistor,
                          float referenceVoltage,
                          uint16_t adcResolution) {
    config.beta = beta;
    config.r0 = nominalResistance;
    config.t0Kelvin = nominalTempC + 273.15f;
    config.seriesResistor = seriesResistor;
    config.vRef = referenceVoltage;
    config.adcResolution = adcResolution;
}

float thermistorVoltageToResistance(const ThermistorConfig &config, float voltage) {
    const float epsilon = 1e-6f;
    float clampedVoltage = signal_conditioning::applySaturation(voltage, epsilon, config.vRef - epsilon);
    float resistance = config.seriesResistor * (config.vRef / clampedVoltage - 1.0f);
    return resistance;
}

float thermistorResistanceToTemperatureC(const ThermistorConfig &config, float resistance) {
    if (resistance <= 0.0f) {
        return -273.15f;
    }

    float invT = (1.0f / config.t0Kelvin) + (1.0f / config.beta) * logf(resistance / config.r0);
    float temperatureKelvin = 1.0f / invT;
    return temperatureKelvin - 273.15f;
}

float thermistorAdcToTemperatureC(const ThermistorConfig &config, uint16_t adcValue) {
    float voltage = signal_conditioning::adcToVoltage(adcValue, config.vRef, config.adcResolution);
    float resistance = thermistorVoltageToResistance(config, voltage);
    return thermistorResistanceToTemperatureC(config, resistance);
}

void thermistorStreamInit(ThermistorStream &stream,
                          const ThermistorConfig &config,
                          uint8_t analogPin,
                          uint8_t oversampleCount) {
    stream.config = config;
    stream.analogPin = analogPin;
    stream.oversampleCount = oversampleCount == 0 ? 1 : oversampleCount;
    stream.lastSample = {0, 0.0f, 0.0f, 0.0f};
    stream.bufferIndex = 0;
    stream.bufferLength = 0;
    memset(stream.buffer, 0, sizeof(stream.buffer));
}

int thermistorStreamGetchar(ThermistorStream &stream) {
    if (stream.bufferIndex >= stream.bufferLength) {
        refreshStreamBuffer(stream);
    }

    char c = stream.buffer[stream.bufferIndex++];
    return static_cast<unsigned char>(c);
}

const ThermistorSample &thermistorStreamGetLastSample(const ThermistorStream &stream) {
    return stream.lastSample;
}

