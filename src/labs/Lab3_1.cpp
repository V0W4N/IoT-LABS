#ifdef LAB_3_1
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "serial_stdio.h"
#include "analog_sensor.h"

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define LED_RING_PIN        6       // LED ring data pin
#define LED_RING_PIXELS     16      // Number of LEDs in ring
#define POT_PIN             A0      // Potentiometer analog input

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
Adafruit_NeoPixel ring(LED_RING_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800);
AnalogSensor potentiometer;

// ============================================================================
// SHARED VARIABLES (Protected by atomic operations or task priorities)
// ============================================================================
volatile double ledStepPosition = 0;
volatile uint16_t potRawValue = 0;
volatile float potVoltage = 0.0;

// System statistics
volatile uint32_t sensorReadCount = 0;
volatile uint32_t ledUpdateCount = 0;

// ============================================================================
// TASK FUNCTION DECLARATIONS
// ============================================================================
void TaskSensorRead(void *pvParameters);
void TaskLedControl(void *pvParameters);
void TaskStatusDisplay(void *pvParameters);

// ============================================================================
// TASK 1: Sensor Reading (Potentiometer ADC)
// ============================================================================
/**
 * Task 1: Periodic Sensor Reading
 * Frequency: 50ms (20Hz) - Fast enough for responsive control
 * 
 * Reads potentiometer value and maps to LED position:
 * - ADC 0-1023 maps to LED positions 0-15
 */
void TaskSensorRead(void *pvParameters) {
    (void) pvParameters;
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(50);  // 50ms period
    
    // Initialize timing
    xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Read sensor data
        analogSensorUpdate(&potentiometer);
        
        // Get readings
        potRawValue = analogSensorGetRaw(&potentiometer);
        potVoltage = analogSensorGetVoltage(&potentiometer);
        
        
        sensorReadCount++;
        
        // Wait until next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}
// ============================================================================
// TASK 2: LED Control
// ============================================================================
/**
 * Task 2: LED Ring Control
 * Frequency: 20ms (50Hz) - Smooth visual updates
 * 
 * Controls LED ring based on sensor input:
 * - Lights single LED at position determined by potentiometer
 * - Creates visual "spinner" effect
 */
void TaskLedControl(void *pvParameters) {
    (void) pvParameters;
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(20);  // 20ms = 50Hz refresh

    xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Update position based on potentiometer (smooth acceleration/deceleration)
        // Center (512) = stationary, left = CCW, right = CW
        int16_t potDelta = potRawValue - 512;
        float speed = (float)potDelta / 1023.0;  // Scale speed: max ±0.5 LED per frame
        ledStepPosition += speed;
        
        // Keep position in reasonable bounds (prevent overflow)
        while (ledStepPosition < 0) ledStepPosition += LED_RING_PIXELS;
        while (ledStepPosition >= LED_RING_PIXELS * 100) ledStepPosition -= LED_RING_PIXELS;
        
        // Extract integer position and fractional offset
        int currentPosition = ((int)floor(ledStepPosition)) % LED_RING_PIXELS;
        float offset = ledStepPosition - floor(ledStepPosition);  // 0.0 to 1.0
        
        // Calculate intensities for smooth fade
        // Current LED fades OUT as we move towards next (bright → dim)
        // Next LED fades IN as we approach it (dim → bright)
        float currentBrightness = 1.0 - offset;  // 1.0 → 0.0
        float nextBrightness = offset;            // 0.0 → 1.0
        
        // Calculate next position with proper wrapping
        int nextPosition = (currentPosition + 1) % LED_RING_PIXELS;
        
        // Rainbow color based on position (color wheel)
        uint8_t hue = (currentPosition * 255) / LED_RING_PIXELS;
        
        // Simple HSV to RGB conversion
        uint8_t r, g, b;
        if (hue < 85) {
            r = 255 - hue * 3;
            g = hue * 3;
            b = 0;
        } else if (hue < 170) {
            hue -= 85;
            r = 0;
            g = 255 - hue * 3;
            b = hue * 3;
        } else {
            hue -= 170;
            r = hue * 3;
            g = 0;
            b = 255 - hue * 3;
        }
        
        // Apply brightness to current LED
        uint8_t currentR = (uint8_t)(r * currentBrightness);
        uint8_t currentG = (uint8_t)(g * currentBrightness);
        uint8_t currentB = (uint8_t)(b * currentBrightness);
        
        // Next LED color (one step ahead in rainbow)
        uint8_t nextHue = (nextPosition * 255) / LED_RING_PIXELS;
        uint8_t nextR, nextG, nextB;
        if (nextHue < 85) {
            nextR = 255 - nextHue * 3;
            nextG = nextHue * 3;
            nextB = 0;
        } else if (nextHue < 170) {
            nextHue -= 85;
            nextR = 0;
            nextG = 255 - nextHue * 3;
            nextB = nextHue * 3;
        } else {
            nextHue -= 170;
            nextR = nextHue * 3;
            nextG = 0;
            nextB = 255 - nextHue * 3;
        }
        
        // Apply brightness to next LED
        nextR = (uint8_t)(nextR * nextBrightness);
        nextG = (uint8_t)(nextG * nextBrightness);
        nextB = (uint8_t)(nextB * nextBrightness);
        
        // Clear all LEDs
        ring.clear();
        
        // Set the two LEDs with smooth crossfade
        ring.setPixelColor(currentPosition, ring.Color(currentR, currentG, currentB));
        ring.setPixelColor(nextPosition, ring.Color(nextR, nextG, nextB));
        ring.show();
        
        ledUpdateCount++;
        
        // Wait for next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// TASK 3: Status Display (STDIO Output)
// ============================================================================
/**
 * Task 3: Simple Status Display
 * Frequency: 200ms (5Hz) - Regular status updates
 * 
 * Displays potentiometer position and LED ring status
 */
void TaskStatusDisplay(void *pvParameters) {
    (void) pvParameters;
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(200);  // 200ms period
    
    xLastWakeTime = xTaskGetTickCount();
    
    // Initial delay to let system stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    while (1) {
        // Read current values (atomic reads of volatiles)
        uint16_t rawAdc = potRawValue;
        uint16_t ledPos = ledStepPosition;
        float voltage = potVoltage;
        
        // Simple output: Potentiometer and LED position
        printf("Pot: %4u (%1.2fV) | LED Position: %2u/%u\r\n", 
               rawAdc,
               (double) voltage,
               ledPos % LED_RING_PIXELS,
               LED_RING_PIXELS - 1);
        
        // Wait for next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================
void setup() {
    // Initialize Serial STDIO (for printf/scanf)
    initSerialStdio(SERIAL_BAUD_RATE);
    
    // Initialize LED ring (NeoPixel)
    ring.begin();
    ring.setBrightness(255);
    ring.clear();
    ring.show();
    
    // Initialize potentiometer sensor
    analogSensorInit(&potentiometer, POT_PIN, 5.0, 1023);
    analogSensorSetFilter(&potentiometer, 4);  // 4-sample averaging
    
    // Create Task 1: Sensor Reading (High priority - input)
    xTaskCreate(TaskSensorRead, "Sensor", 128, NULL, 3, NULL);
    
    // Create Task 2: LED Control (High priority - output)
    xTaskCreate(TaskLedControl, "LED", 128, NULL, 2, NULL);
    
    // Create Task 3: Status Display (Low priority - monitoring)
    xTaskCreate(TaskStatusDisplay, "Display", 256, NULL, 1, NULL);
    
    printf("Lab 3.1: LED Ring Control Ready\r\n");
    
    // Start FreeRTOS scheduler
    vTaskStartScheduler();
    
    // Should never reach here
    while(1);
}

// ============================================================================
// ARDUINO LOOP (Not used - FreeRTOS takes control)
// ============================================================================
void loop() {
    // Empty - FreeRTOS scheduler handles all tasks
}

#endif
