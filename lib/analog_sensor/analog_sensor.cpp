#include "analog_sensor.h"

void analogSensorInit(AnalogSensor *sensor, uint8_t pin, float refVoltage, uint16_t adcRes) {
    sensor->pin = pin;
    sensor->referenceVoltage = refVoltage;
    sensor->adcResolution = adcRes;
    sensor->rawValue = 0;
    sensor->voltage = 0.0;
    sensor->scaledValue = 0.0;
    sensor->scaleMin = 0.0;
    sensor->scaleMax = 100.0;  // Default: 0-100 scale
    sensor->filterSamples = 1;  // No filtering by default
    
    // Configure pin as input
    pinMode(pin, INPUT);
}

void analogSensorSetScale(AnalogSensor *sensor, float minValue, float maxValue) {
    sensor->scaleMin = minValue;
    sensor->scaleMax = maxValue;
}

void analogSensorSetFilter(AnalogSensor *sensor, uint8_t samples) {
    if (samples > 0) {
        sensor->filterSamples = samples;
    }
}

uint16_t analogSensorReadRaw(AnalogSensor *sensor) {
    if (sensor->filterSamples == 1) {
        // No filtering
        sensor->rawValue = analogRead(sensor->pin);
    } else {
        // Average multiple samples
        uint32_t sum = 0;
        for (uint8_t i = 0; i < sensor->filterSamples; i++) {
            sum += analogRead(sensor->pin);
            delayMicroseconds(100);  // Small delay between samples
        }
        sensor->rawValue = sum / sensor->filterSamples;
    }
    
    return sensor->rawValue;
}

float analogSensorReadVoltage(AnalogSensor *sensor) {
    analogSensorReadRaw(sensor);
    sensor->voltage = (sensor->rawValue * sensor->referenceVoltage) / sensor->adcResolution;
    return sensor->voltage;
}

float analogSensorReadScaled(AnalogSensor *sensor) {
    analogSensorReadRaw(sensor);
    
    // Map raw value to scaled range
    float normalized = (float)sensor->rawValue / sensor->adcResolution;
    sensor->scaledValue = sensor->scaleMin + (normalized * (sensor->scaleMax - sensor->scaleMin));
    
    return sensor->scaledValue;
}

void analogSensorUpdate(AnalogSensor *sensor) {
    // Read raw value
    analogSensorReadRaw(sensor);
    
    // Calculate voltage
    sensor->voltage = (sensor->rawValue * sensor->referenceVoltage) / sensor->adcResolution;
    
    // Calculate scaled value
    float normalized = (float)sensor->rawValue / sensor->adcResolution;
    sensor->scaledValue = sensor->scaleMin + (normalized * (sensor->scaleMax - sensor->scaleMin));
}

uint16_t analogSensorGetRaw(AnalogSensor *sensor) {
    return sensor->rawValue;
}

float analogSensorGetVoltage(AnalogSensor *sensor) {
    return sensor->voltage;
}

float analogSensorGetScaled(AnalogSensor *sensor) {
    return sensor->scaledValue;
}

