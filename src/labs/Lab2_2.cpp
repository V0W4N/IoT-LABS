#ifdef LAB_2_2
#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include <queue.h>
#include "config.h"
#include "serial_stdio.h"

// Lab 2.2 - FreeRTOS: Preemptive Multitasking with Semaphores and Queues
// Three tasks demonstrating synchronization and inter-task communication

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define BTN1_PIN 2          // Button for Task 1
#define LED1_PIN 13         // LED controlled by Task 1 (1 second on button press)
#define LED2_PIN 12         // LED controlled by Task 2 (blinks N times)

// ============================================================================
// FREERTOS OBJECTS
// ============================================================================
SemaphoreHandle_t buttonSemaphore;  // Binary semaphore for Task 1 -> Task 2 sync
QueueHandle_t dataQueue;            // Queue for Task 2 -> Task 3 communication

// ============================================================================
// SHARED VARIABLES
// ============================================================================
volatile int N = 0;                 // Counter incremented by Task 2
volatile unsigned long led1_onTime = 0;  // Time when LED1 should turn off

// ============================================================================
// TASK FUNCTION DECLARATIONS
// ============================================================================
void Task1_ButtonAndLED(void *pvParameters);
void Task2_SynchronousTask(void *pvParameters);
void Task3_BufferReader(void *pvParameters);

// ============================================================================
// TASK 1: Button and LED Control
// ============================================================================
/**
 * Task 1: Button Monitoring and LED Control
 * - Frequency: 10ms (100Hz)
 * - Detects button press with debouncing
 * - Lights LED1 for 1 second on button press
 * - Signals Task 2 via binary semaphore
 */
void Task1_ButtonAndLED(void *pvParameters) {
    (void) pvParameters;  // Unused parameter
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(10);  // 10ms period
    
    // Button debouncing variables
    bool btnLastState = HIGH;
    unsigned long btnLastDebounce = 0;
    const unsigned long DEBOUNCE_DELAY = 50;
    
    // Initialize the xLastWakeTime variable with the current time
    xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        // Read button with debouncing
        bool btnReading = digitalRead(BTN1_PIN);
        
        if (btnReading != btnLastState) {
            btnLastDebounce = millis();
        }
        
        if ((millis() - btnLastDebounce) > DEBOUNCE_DELAY) {
            if (btnReading == LOW && btnLastState == HIGH) {
                // Button pressed (falling edge)
                
                // Turn on LED1 for 1 second
                digitalWrite(LED1_PIN, HIGH);
                led1_onTime = millis() + 1000;  // LED should turn off after 1 second
                
                // Signal Task 2 via semaphore
                xSemaphoreGive(buttonSemaphore);
                
                printf("Task1: Button pressed! LED1 ON, semaphore given.\r\n");
            }
        }
        
        btnLastState = btnReading;
        
        // Check if LED1 should be turned off
        if (led1_onTime > 0 && millis() >= led1_onTime) {
            digitalWrite(LED1_PIN, LOW);
            led1_onTime = 0;
            printf("Task1: LED1 OFF after 1 second.\r\n");
        }
        
        // Wait for the next cycle (10ms)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// TASK 2: Synchronous Task (Provider)
// ============================================================================
/**
 * Task 2: Synchronous Data Provider
 * - Waits for semaphore from Task 1
 * - Increments counter N
 * - Sends bytes (1,2,3,...N) to queue with 50ms interval
 * - Blinks LED2 N times (ON: 300ms, OFF: 500ms)
 */
void Task2_SynchronousTask(void *pvParameters) {
    (void) pvParameters;  // Unused parameter
    
    while (1) {
        // Wait for semaphore from Task 1 (blocking wait)
        if (xSemaphoreTake(buttonSemaphore, portMAX_DELAY) == pdTRUE) {
            // Semaphore received - button was pressed
            N++;
            printf("Task2: Semaphore received! N = %d\r\n", N);
            
            // Send series of bytes (1,2,3,...N) to queue with 50ms interval
            printf("Task2: Sending bytes to queue: ");
            for (int i = 1; i <= N; i++) {
                uint8_t byte = i;
                
                // Send byte to queue (non-blocking or with timeout)
                if (xQueueSendToBack(dataQueue, &byte, pdMS_TO_TICKS(10)) == pdTRUE) {
                    printf("%d ", byte);
                } else {
                    printf("(queue full!) ");
                }
                
                vTaskDelay(pdMS_TO_TICKS(50));  // 50ms delay between bytes
            }
            
            // Send terminator byte (0) for newline
            uint8_t terminator = 0;
            xQueueSendToBack(dataQueue, &terminator, pdMS_TO_TICKS(10));
            printf("0\r\n");
            
            // Blink LED2 N times (ON: 300ms, OFF: 500ms)
            printf("Task2: Blinking LED2 %d times.\r\n", N);
            for (int i = 0; i < N; i++) {
                digitalWrite(LED2_PIN, HIGH);
                vTaskDelay(pdMS_TO_TICKS(300));  // ON for 300ms
                
                digitalWrite(LED2_PIN, LOW);
                vTaskDelay(pdMS_TO_TICKS(500));  // OFF for 500ms
            }
            
            printf("Task2: Finished sequence for N=%d.\r\n", N);
        }
    }
}

// ============================================================================
// TASK 3: Asynchronous Buffer Reader (Consumer)
// ============================================================================
/**
 * Task 3: Asynchronous Data Consumer
 * - Frequency: 200ms (5Hz)
 * - Reads bytes from queue
 * - Displays bytes in terminal
 * - Prints newline when byte 0 is received
 */
void Task3_BufferReader(void *pvParameters) {
    (void) pvParameters;  // Unused parameter
    
    TickType_t xLastWakeTime;
    const TickType_t xFrequency = pdMS_TO_TICKS(200);  // 200ms period
    
    // Initialize the xLastWakeTime variable with the current time
    xLastWakeTime = xTaskGetTickCount();
    
    while (1) {
        uint8_t receivedByte;
        
        // Try to receive data from queue (non-blocking check)
        while (xQueueReceive(dataQueue, &receivedByte, 0) == pdTRUE) {
            if (receivedByte == 0) {
                // Terminator byte - newline
                printf("\r\n");
            } else {
                // Regular byte - print it
                printf("%d ", receivedByte);
            }
        }
        
        // Wait for next cycle (200ms)
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================
void setup() {
    // Initialize Serial for STDIO
    initSerialStdio(SERIAL_BAUD_RATE);
    
    printf("Lab 2.2 - FreeRTOS Multitasking\r\n");
    printf("================================\r\n\r\n");
    
    // Configure pins
    pinMode(BTN1_PIN, INPUT_PULLUP);
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    
    // Initialize LEDs to OFF
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    
    // Create binary semaphore for Task 1 -> Task 2 synchronization
    buttonSemaphore = xSemaphoreCreateBinary();
    if (buttonSemaphore == NULL) {
        printf("ERROR: Failed to create semaphore!\r\n");
        while(1);  // Halt on error
    }
    
    // Create queue for Task 2 -> Task 3 communication (10 bytes capacity)
    dataQueue = xQueueCreate(10, sizeof(uint8_t));
    if (dataQueue == NULL) {
        printf("ERROR: Failed to create queue!\r\n");
        while(1);  // Halt on error
    }
    
    printf("FreeRTOS objects created successfully.\r\n");
    printf("Creating tasks...\r\n\r\n");
    
    // Create Task 1: Button and LED (Priority 2 - high)
    xTaskCreate(
        Task1_ButtonAndLED,      // Task function
        "Task1_Button",          // Task name (for debugging)
        128,                     // Stack size (words)
        NULL,                    // Task parameters
        2,                       // Priority (higher = more important)
        NULL                     // Task handle
    );
    
    // Create Task 2: Synchronous Task (Priority 1 - medium)
    xTaskCreate(
        Task2_SynchronousTask,
        "Task2_Sync",
        256,                     // Larger stack for printf operations
        NULL,
        1,
        NULL
    );
    
    // Create Task 3: Buffer Reader (Priority 1 - medium)
    xTaskCreate(
        Task3_BufferReader,
        "Task3_Reader",
        128,
        NULL,
        1,
        NULL
    );
    
    printf("All tasks created!\r\n");
    printf("BTN1 (pin %d): Press to trigger sequence\r\n", BTN1_PIN);
    printf("LED1 (pin %d): Lights for 1 second on button press\r\n", LED1_PIN);
    printf("LED2 (pin %d): Blinks N times after sequence\r\n\r\n", LED2_PIN);
    printf("Starting scheduler...\r\n\r\n");
    
    // Start the FreeRTOS scheduler
    // This function never returns if scheduler starts successfully
    vTaskStartScheduler();
    
    // Should never reach here
    printf("ERROR: Scheduler failed to start!\r\n");
    while(1);
}

// ============================================================================
// ARDUINO LOOP (Not used - FreeRTOS takes control)
// ============================================================================
void loop() {
    // Empty - FreeRTOS scheduler handles all tasks
    // This function is never called after vTaskStartScheduler()
}

#endif
