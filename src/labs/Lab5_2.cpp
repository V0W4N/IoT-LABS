#ifdef LAB_5_2

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>
#include <math.h>
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

// Smooth following parameters (exponential smoothing)
constexpr float SMOOTHING_FACTOR = 0.15f;  // 0.0 = no change, 1.0 = instant (0.1-0.2 is smooth)
constexpr float DEADBAND_DEGREES = 1.0f;   // Ignore changes smaller than this

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
volatile int16_t targetAngle = 0;  // Target angle from potentiometer

// Smooth following state
static float smoothedAngle = 0.0f;     // Smoothed servo angle (float for precision)

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
// Task 2: Servo Control (Exponential Smoothing)
// -----------------------------------------------------------------------------
/**
 * Task 2: Servo Control with Smooth Following
 * Frequency: 20ms (50Hz) - Smooth servo updates
 * 
 * Uses exponential smoothing to follow potentiometer position smoothly
 * Maps potentiometer value (0-1023) to target angle (0-180 degrees)
 * Smoothing prevents jitter from ADC noise and provides nice movement
 */
void TaskServoControl(void *pvParameters) {
    (void) pvParameters;
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(20);  // 20ms = 50Hz refresh

    xLastWakeTime = xTaskGetTickCount();
    
    // Initialize smoothed angle from current potentiometer reading
    uint16_t rawAdc = potRawValue;
    targetAngle = map(rawAdc, 0, 1023, 0, 180);
    smoothedAngle = static_cast<float>(targetAngle);
    servoAngle = targetAngle;
    servo_set_angle(&gServo, servoAngle);
    
    while (1) {
        // Read current potentiometer value and calculate target angle
        rawAdc = potRawValue;
        int16_t newTargetAngle = map(rawAdc, 0, 1023, 0, 180);
        targetAngle = newTargetAngle;
        
        // Calculate error between target and smoothed position
        float error = static_cast<float>(targetAngle) - smoothedAngle;
        
        // Apply deadband to prevent jitter from ADC noise
        if (fabs(error) > DEADBAND_DEGREES) {
            // Exponential smoothing: smoothed = smoothed + factor * (target - smoothed)
            smoothedAngle += SMOOTHING_FACTOR * error;
        }
        
        // Clamp to valid servo range (0-180 degrees)
        if (smoothedAngle < 0.0f) {
            smoothedAngle = 0.0f;
        } else if (smoothedAngle > 180.0f) {
            smoothedAngle = 180.0f;
        }
        
        // Convert to integer angle for servo
        int16_t finalAngle = static_cast<int16_t>(smoothedAngle + 0.5f);  // Round to nearest
        
        // Only update servo if angle actually changed (reduces PWM jitter)
        if (finalAngle != servoAngle) {
            servo_set_angle(&gServo, finalAngle);
            servoAngle = finalAngle;
        }
        
        servoUpdateCount++;
        
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

    fprintf(&gLcdStream, "\fLab 5.2 Ready\nInit FreeRTOS...");
    printf("Lab 5.2: Smooth Servo Control System Ready\r\n");
    printf("Potentiometer controls servo angle (0-180 degrees)\r\n");
    printf("Smoothing: factor=%.2f, deadband=%.1f deg\r\n", 
           (double)SMOOTHING_FACTOR, (double)DEADBAND_DEGREES);

    // Create FreeRTOS tasks
    // Priority order: SensorRead (3) > ServoControl (2) > StatusDisplay (1) > StatusLED (0)
    xTaskCreate(TaskSensorRead, "Sensor", 128, nullptr, 3, nullptr);
    xTaskCreate(TaskServoControl, "ServoCtrl", 128, nullptr, 2, nullptr);
    xTaskCreate(TaskStatusDisplay, "StatusDisp", 256, nullptr, 1, nullptr);
    xTaskCreate(TaskStatusLED, "StatusLED", 128, nullptr, 0, nullptr);

    printf("FreeRTOS scheduler starting...\r\n");
    fprintf(&gLcdStream, "\fLab 5.2 Ready\nFreeRTOS active");

    vTaskStartScheduler();

    // Should never reach here
    while (true) {
        // Error handler
    }
}

void loop() {
    // Not used - FreeRTOS scheduler handles execution
}

#endif  // LAB_5_2
