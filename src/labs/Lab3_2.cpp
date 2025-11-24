#ifdef LAB_3_2

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <Wire.h>
#include <semphr.h>
#include <stdio.h>

#include "config.h"
#include "signal_conditioning.h"
#include "thermistor_utils.h"
#include "lcd_stdio.h"

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
constexpr uint8_t THERMISTOR_PIN = A0;
constexpr uint8_t STATUS_LED_PIN = 8;

// LCD parameters (16x2 I2C display at 0x27)
constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;

// -----------------------------------------------------------------------------
// Sensor & filter configuration
// -----------------------------------------------------------------------------
constexpr float THERMISTOR_BETA = 3950.0f;
constexpr float THERMISTOR_NOMINAL_RESISTANCE = 10000.0f;  // 10k at 25°C
constexpr float THERMISTOR_NOMINAL_TEMP_C = 25.0f;
constexpr float THERMISTOR_SERIES_RESISTOR = 10000.0f;     // 10k pull-up
constexpr float ADC_REFERENCE_VOLTAGE = 5.0f;
constexpr uint16_t ADC_RESOLUTION = 1023;

constexpr float TEMPERATURE_MIN_C = -40.0f;
constexpr float TEMPERATURE_MAX_C = 125.0f;
constexpr float TEMPERATURE_HIGH_THRESHOLD_C = 28.0f;
constexpr float TEMPERATURE_LOW_THRESHOLD_C = 10.0f;

constexpr TickType_t SENSOR_SAMPLE_PERIOD = pdMS_TO_TICKS(100);   // 10 Hz
constexpr TickType_t REPORT_PERIOD = pdMS_TO_TICKS(500);          // 2 Hz

constexpr size_t MEDIAN_WINDOW = 5;
constexpr float WMA_WEIGHTS[] = {0.4f, 0.3f, 0.2f, 0.1f};
constexpr size_t WMA_TAPS = sizeof(WMA_WEIGHTS) / sizeof(WMA_WEIGHTS[0]);

// -----------------------------------------------------------------------------
// Data structures
// -----------------------------------------------------------------------------
struct ConditionedSample {
    uint16_t rawAdc;
    float voltage;
    float resistance;
    float temperatureRawC;
    float temperatureFilteredC;
    bool alarmHigh;
};

static ConditionedSample gLatestSample{};
static SemaphoreHandle_t gSampleMutex = nullptr;

static signal_conditioning::SaltPepperFilter gMedianFilter{};
static signal_conditioning::WeightedMovingAverage gWeightedFilter{};
static ThermistorConfig gThermistorConfig{};
static ThermistorStream gThermistorStream{};

static volatile uint32_t gTotalSamples = 0;
static volatile uint32_t gReportCount = 0;

static FILE gLabStream;

static int lcdStreamPutchar(char c, FILE *file) {
    return LCDStdio::putcharlcd(c, file);
}

static int thermistorStreamGetcharShim(FILE *file) {
    return thermistorStreamGetchar(gThermistorStream);
}

// -----------------------------------------------------------------------------
// Utility helpers
// -----------------------------------------------------------------------------
static ConditionedSample getLatestSampleSnapshot() {
    ConditionedSample snapshot{};
    if (gSampleMutex != nullptr && xSemaphoreTake(gSampleMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        snapshot = gLatestSample;
        xSemaphoreGive(gSampleMutex);
    }
    return snapshot;
}

static void updateLatestSample(const ConditionedSample &sample) {
    if (gSampleMutex == nullptr) {
        return;
    }
    if (xSemaphoreTake(gSampleMutex, portMAX_DELAY) == pdTRUE) {
        gLatestSample = sample;
        xSemaphoreGive(gSampleMutex);
    }
}

static void updateLcd(const ConditionedSample &sample) {
    const char *statusText = "OK";
    if (sample.alarmHigh) {
        statusText = "HIGH";
    } else if (sample.temperatureFilteredC <= TEMPERATURE_LOW_THRESHOLD_C) {
        statusText = "LOW";
    }

    printf(
        "\fT:%5.1f%cC %-4s\nADC:%4u V:%1.2f",
        static_cast<double>(sample.temperatureFilteredC),
        (char)223,
        statusText,
        sample.rawAdc,
        static_cast<double>(sample.voltage));
}

// -----------------------------------------------------------------------------
// FreeRTOS Tasks
// -----------------------------------------------------------------------------
void TaskSensorPipeline(void *pvParameters) {
    (void) pvParameters;

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        uint16_t rawAdc = 0;
        if (scanf("%hu", &rawAdc) != 1) {
            rawAdc = thermistorStreamGetLastSample(gThermistorStream).adcValue;
        }

        float medianAdc = signal_conditioning::saltPepperProcess(gMedianFilter, static_cast<float>(rawAdc));
        uint16_t filteredAdc = static_cast<uint16_t>(medianAdc + 0.5f);

        float voltage = signal_conditioning::adcToVoltage(filteredAdc, ADC_REFERENCE_VOLTAGE, ADC_RESOLUTION);
        float resistance = thermistorVoltageToResistance(gThermistorConfig, voltage);
        float temperatureRawC = thermistorResistanceToTemperatureC(gThermistorConfig, resistance);

        float temperatureFilteredC = signal_conditioning::weightedMovingAverageProcess(gWeightedFilter, temperatureRawC);
        temperatureFilteredC = signal_conditioning::applySaturation(
            temperatureFilteredC, TEMPERATURE_MIN_C, TEMPERATURE_MAX_C);

        bool alarmHigh = temperatureFilteredC >= TEMPERATURE_HIGH_THRESHOLD_C;
        digitalWrite(STATUS_LED_PIN, alarmHigh ? HIGH : LOW);

        ConditionedSample sample{
            .rawAdc = filteredAdc,
            .voltage = voltage,
            .resistance = resistance,
            .temperatureRawC = temperatureRawC,
            .temperatureFilteredC = temperatureFilteredC,
            .alarmHigh = alarmHigh
        };

        updateLatestSample(sample);
        gTotalSamples++;

        vTaskDelayUntil(&lastWakeTime, SENSOR_SAMPLE_PERIOD);
    }
}

void TaskReporter(void *pvParameters) {
    (void) pvParameters;

    // Offset start to avoid contention
    vTaskDelay(pdMS_TO_TICKS(250));

    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        ConditionedSample sample = getLatestSampleSnapshot();

        updateLcd(sample);
        gReportCount++;

        vTaskDelayUntil(&lastWakeTime, REPORT_PERIOD);
    }
}

// -----------------------------------------------------------------------------
// Arduino setup & loop
// -----------------------------------------------------------------------------
void setup() {
    pinMode(THERMISTOR_PIN, INPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);

    Wire.begin();

    LCDStdio::init(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
    LCDStdio::clear();

    thermistorConfigInit(gThermistorConfig,
                         THERMISTOR_BETA,
                         THERMISTOR_NOMINAL_RESISTANCE,
                         THERMISTOR_NOMINAL_TEMP_C,
                         THERMISTOR_SERIES_RESISTOR,
                         ADC_REFERENCE_VOLTAGE,
                         ADC_RESOLUTION);

    signal_conditioning::saltPepperInit(gMedianFilter, MEDIAN_WINDOW);
    signal_conditioning::weightedMovingAverageInit(gWeightedFilter, WMA_WEIGHTS, WMA_TAPS);
    thermistorStreamInit(gThermistorStream, gThermistorConfig, THERMISTOR_PIN, MEDIAN_WINDOW);

    fdev_setup_stream(&gLabStream, lcdStreamPutchar, thermistorStreamGetcharShim, _FDEV_SETUP_RW);
    stdout = &gLabStream;
    stdin = &gLabStream;
    stderr = &gLabStream;

    gSampleMutex = xSemaphoreCreateMutex();

    // Preload LCD with startup message via stdio stream
    printf("\fLab 3.2 Ready\nInit filters...");

    xTaskCreate(TaskSensorPipeline, "Sensor", 256, nullptr, 3, nullptr);
    xTaskCreate(TaskReporter, "Reporter", 256, nullptr, 2, nullptr);

    printf("\fLab 3.2 Ready\nScheduler start");

    vTaskStartScheduler();

    while (true) {
        // Should never reach here
    }
}

void loop() {
    // Not used - FreeRTOS scheduler handles execution
}

#endif  // LAB_3_2
