#ifndef ANALOG_SENSOR_H
#define ANALOG_SENSOR_H

#include <Arduino.h>

/**
 * Analog Sensor Reading Library
 * 
 * Provides interface for reading and processing analog sensor data
 * Features: raw reading, voltage conversion, custom scaling, filtering
 */

typedef struct {
    uint8_t pin;
    uint16_t rawValue;
    float voltage;
    float scaledValue;
    uint16_t adcResolution;    // ADC resolution (e.g., 1023 for 10-bit)
    float referenceVoltage;    // Reference voltage (e.g., 5.0V or 3.3V)
    float scaleMin;            // Minimum scaled value
    float scaleMax;            // Maximum scaled value
    uint8_t filterSamples;     // Number of samples for averaging filter
} AnalogSensor;

/**
 * Initialize analog sensor
 * @param sensor Pointer to AnalogSensor structure
 * @param pin Analog pin number (A0, A1, etc.)
 * @param refVoltage Reference voltage (typically 5.0V for Arduino Uno)
 * @param adcRes ADC resolution (1023 for 10-bit ADC)
 */
void analogSensorInit(AnalogSensor *sensor, uint8_t pin, float refVoltage, uint16_t adcRes);

/**
 * Set scaling parameters for converting raw ADC to physical units
 * @param sensor Pointer to AnalogSensor structure
 * @param minValue Minimum scaled value (at ADC = 0)
 * @param maxValue Maximum scaled value (at ADC = max)
 */
void analogSensorSetScale(AnalogSensor *sensor, float minValue, float maxValue);

/**
 * Set number of samples for averaging filter
 * @param sensor Pointer to AnalogSensor structure
 * @param samples Number of samples to average (1 = no filtering)
 */
void analogSensorSetFilter(AnalogSensor *sensor, uint8_t samples);

/**
 * Read raw ADC value
 * @param sensor Pointer to AnalogSensor structure
 * @return Raw ADC value (0-1023 for 10-bit ADC)
 */
uint16_t analogSensorReadRaw(AnalogSensor *sensor);

/**
 * Read and convert to voltage
 * @param sensor Pointer to AnalogSensor structure
 * @return Voltage value (0 - refVoltage)
 */
float analogSensorReadVoltage(AnalogSensor *sensor);

/**
 * Read and convert to scaled value
 * @param sensor Pointer to AnalogSensor structure
 * @return Scaled value (scaleMin - scaleMax)
 */
float analogSensorReadScaled(AnalogSensor *sensor);

/**
 * Update all sensor readings (raw, voltage, scaled)
 * @param sensor Pointer to AnalogSensor structure
 */
void analogSensorUpdate(AnalogSensor *sensor);

/**
 * Get last raw reading without new read
 * @param sensor Pointer to AnalogSensor structure
 * @return Last raw value
 */
uint16_t analogSensorGetRaw(AnalogSensor *sensor);

/**
 * Get last voltage reading without new read
 * @param sensor Pointer to AnalogSensor structure
 * @return Last voltage value
 */
float analogSensorGetVoltage(AnalogSensor *sensor);

/**
 * Get last scaled reading without new read
 * @param sensor Pointer to AnalogSensor structure
 * @return Last scaled value
 */
float analogSensorGetScaled(AnalogSensor *sensor);

#endif

