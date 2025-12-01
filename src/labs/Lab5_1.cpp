#ifdef LAB_5_1

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>
#include <Wire.h>

#include "config.h"
#include "serial_stdio.h"
#include "analog_sensor.h"
#include "my_servo.h"
#include "lcd_stdio.h"

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
constexpr uint8_t SERVO_PIN = 2;
constexpr uint8_t POT_PIN = A0;
constexpr uint8_t STATUS_LED_PIN = 13;

constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;

constexpr TickType_t STATUS_UPDATE_PERIOD = pdMS_TO_TICKS(500);
constexpr TickType_t LED_BLINK_PERIOD = pdMS_TO_TICKS(1000);

// Hysteresis threshold (in ADC counts) to prevent jitter
// ~5 ADC counts ≈ 0.88 degrees of servo movement
constexpr uint16_t POT_HYSTERESIS_THRESHOLD = 250;

// -----------------------------------------------------------------------------
// Global objects
// -----------------------------------------------------------------------------
static AnalogSensor gPotentiometer;
static ServoMotor gServo;
static FILE gLcdStream;

// -----------------------------------------------------------------------------
// Shared variables (protected by atomic operations or task priorities)
// -----------------------------------------------------------------------------
volatile uint16_t potRawValue = 0;
volatile float potVoltage = 0.0;
volatile int16_t servoAngle = 0;

// Hysteresis tracking (local to servo control task)
static uint16_t lastPotValue = 0;  // Last potentiometer value that triggered servo update

// System statistics
volatile uint32_t sensorReadCount = 0;
volatile uint32_t servoUpdateCount = 0;

// -----------------------------------------------------------------------------
// Helper functions
// -----------------------------------------------------------------------------
static int lcdStreamPutchar(char c, FILE *file) {
    return LCDStdio::putcharlcd(c, file);
}

static void updateStatusDisplay() {
    // Read current values (atomic reads of volatiles)
    uint16_t rawAdc = potRawValue;
    float voltage = potVoltage;
    int16_t angle = servoAngle;
    
    fprintf(&gLcdStream,
            "\fPot: %4u (%1.2fV)\nServo: %3d deg",
            rawAdc,
            (double)voltage,
            angle);
}


// -----------------------------------------------------------------------------
// FreeRTOS Task declarations
// -----------------------------------------------------------------------------
void TaskSensorRead(void *pvParameters);
void TaskServoControl(void *pvParameters);
void TaskStatusDisplay(void *pvParameters);
void TaskStatusLED(void *pvParameters);

// -----------------------------------------------------------------------------
// Task 1: Sensor Reading (Potentiometer ADC)
// -----------------------------------------------------------------------------
/**
 * Task 1: Periodic Sensor Reading
 * Frequency: 50ms (20Hz) - Fast enough for responsive control
 * 
 * Reads potentiometer value and updates shared variables
 */
void TaskSensorRead(void *pvParameters) {
    (void) pvParameters;
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(50);  // 50ms period
    
    // Initialize timing
    xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Read sensor data
        analogSensorUpdate(&gPotentiometer);
        
        // Get readings
        potRawValue = analogSensorGetRaw(&gPotentiometer);
        potVoltage = analogSensorGetVoltage(&gPotentiometer);
        
        sensorReadCount++;
        
        // Wait until next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// -----------------------------------------------------------------------------
// Task 2: Servo Control (Map potentiometer to servo angle)
// -----------------------------------------------------------------------------
/**
 * Task 2: Servo Control
 * Frequency: 20ms (50Hz) - Smooth servo updates
 * 
 * Maps potentiometer value (0-1023) to servo angle (0-180 degrees)
 * Uses hysteresis to prevent jitter from small potentiometer fluctuations
 */
void TaskServoControl(void *pvParameters) {
    (void) pvParameters;
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(20);  // 20ms = 50Hz refresh

    xLastWakeTime = xTaskGetTickCount();
    
    // Initialize hysteresis tracking
    lastPotValue = potRawValue;
    
    while (1) {
        // Read current potentiometer value
        uint16_t rawAdc = potRawValue;
        
        // Calculate absolute difference from last update
        uint16_t potDelta = (rawAdc > lastPotValue) ? 
                           (rawAdc - lastPotValue) : 
                           (lastPotValue - rawAdc);
        
        // Only update servo if change exceeds hysteresis threshold
        if (potDelta >= POT_HYSTERESIS_THRESHOLD) {
            // Map ADC value (0-1023) to servo angle (0-180 degrees)
            int16_t angle = map(rawAdc, 0, 1023, 0, 180);
            
            // Update servo angle
            servo_set_angle(&gServo, angle);
            servoAngle = angle;
            
            // Update last potentiometer value that triggered update
            lastPotValue = rawAdc;
            
            servoUpdateCount++;
        }
        
        // Wait for next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// -----------------------------------------------------------------------------
// Task 3: Status Display (LCD and Serial updates)
// -----------------------------------------------------------------------------
void TaskStatusDisplay(void *pvParameters) {
    (void) pvParameters;
    vTaskDelay(pdMS_TO_TICKS(250));
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        updateStatusDisplay();
        vTaskDelayUntil(&lastWakeTime, STATUS_UPDATE_PERIOD);
    }
}

// -----------------------------------------------------------------------------
// Task 4: Status LED Blink (System heartbeat)
// -----------------------------------------------------------------------------
void TaskStatusLED(void *pvParameters) {
    (void) pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    bool ledState = false;

    for (;;) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        vTaskDelayUntil(&lastWakeTime, LED_BLINK_PERIOD);
    }
}

// -----------------------------------------------------------------------------
// Arduino setup & loop
// -----------------------------------------------------------------------------
void setup() {
    // Initialize Serial STDIO (for printf/scanf)
    initSerialStdio(SERIAL_BAUD_RATE);
    
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    // Initialize potentiometer sensor
    analogSensorInit(&gPotentiometer, POT_PIN, 5.0, 1023);
    analogSensorSetFilter(&gPotentiometer, 4);  // 4-sample averaging
    
    // Initialize servo motor
    servo_init(&gServo, SERVO_PIN);
    servo_attach(&gServo);
    servo_set_angle(&gServo, 0);  // Start at 0 degrees
    servoAngle = 0;

    // Initialize LCD
    Wire.begin();
    LCDStdio::init(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
    LCDStdio::clear();
    fdev_setup_stream(&gLcdStream, lcdStreamPutchar, nullptr, _FDEV_SETUP_WRITE);

    fprintf(&gLcdStream, "\fLab 5.1 Ready\nInit FreeRTOS...");
    printf("Lab 5.1: Servo Control System Ready\r\n");
    printf("Potentiometer controls servo angle (0-180 degrees)\r\n");

    // Create FreeRTOS tasks
    // Priority order: SensorRead (3) > ServoControl (2) > StatusDisplay (1) > StatusLED (0)
    xTaskCreate(TaskSensorRead, "Sensor", 128, nullptr, 3, nullptr);
    xTaskCreate(TaskServoControl, "ServoCtrl", 128, nullptr, 2, nullptr);
    xTaskCreate(TaskStatusDisplay, "StatusDisp", 256, nullptr, 1, nullptr);
    xTaskCreate(TaskStatusLED, "StatusLED", 128, nullptr, 0, nullptr);

    printf("FreeRTOS scheduler starting...\r\n");
    fprintf(&gLcdStream, "\fLab 5.1 Ready\nFreeRTOS active");

    vTaskStartScheduler();

    // Should never reach here
    while (true) {
        // Error handler
    }
}

void loop() {
    // Not used - FreeRTOS scheduler handles execution
}

#endif  // LAB_5_1
