#ifdef LAB_2_1
#include <Arduino.h>
#include "config.h"
#include "serial_stdio.h"

// Lab 2.1 - Sequential Task Execution with Provider/Consumer Model
// Non-preemptive scheduler with 3 tasks and idle reporting

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define BTN1_PIN 2          // Button for Task 1 (toggle LED1)
#define BTN2_PIN 3          // Button for Task 3 (increment counter)
#define BTN3_PIN 4          // Button for Task 3 (decrement counter)
#define LED1_PIN 13         // LED for Task 1 (toggle on button press)
#define LED2_PIN 12         // LED for Task 2 (blink when LED1 is OFF)


// ============================================================================
// TASK STRUCTURE
// ============================================================================
struct Task {
    unsigned long period;      // Period in milliseconds (recurrence)
    unsigned long offset;      // Offset from start (initial delay)
    unsigned long lastRun;     // Last execution time
    bool firstRun;             // Flag for first execution
};

// ============================================================================
// GLOBAL VARIABLES (Provider/Consumer Model)
// ============================================================================
// Task 1 state (Provider)
bool led1_state = false;           // Current state of LED1

// Task 2 state (Provider)
bool led2_state = false;           // Current state of LED2
unsigned long led2_onTime = 0;     // Time LED2 has been ON (ms)
unsigned long led2_offTime = 0;    // Time LED2 has been OFF (ms)
unsigned long led2_lastChange = 0; // Last time LED2 changed state

// Task 3 state (Provider)
int counter = 0;                   // Counter value

// Button states (for debouncing)
bool btnLastStates[3] = {HIGH, HIGH, HIGH}; // BTN1, BTN2, BTN3
bool btnLastStableStates[3] = {HIGH, HIGH, HIGH};
unsigned long btnLastDebounceTime[3] = {0, 0, 0};
const unsigned long DEBOUNCE_DELAY = 50;

// Helper function to get button index from pin
int getButtonIndex(int btnPin) {
    switch(btnPin) {
        case BTN1_PIN: return 0;
        case BTN2_PIN: return 1;
        case BTN3_PIN: return 2;
        default: return -1;
    }
}

bool debounceButton(int btnPin, bool riseEdgeMode = false) {
    int btnIndex = getButtonIndex(btnPin);
    if (btnIndex < 0) return false; // Invalid pin

    bool btnReading = digitalRead(btnPin);
    
    bool isFallEdge = false;
    bool isRiseEdge = false;
    
    // Reset timer on any state change
    if (btnReading != btnLastStates[btnIndex]) {
        btnLastDebounceTime[btnIndex] = millis();
    }
    
    // Only act if signal has been stable for DEBOUNCE_DELAY
    if ((millis() - btnLastDebounceTime[btnIndex]) > DEBOUNCE_DELAY) {
        // Check for falling edge (button press)
        if (btnReading == LOW && btnLastStableStates[btnIndex] == HIGH) {
            btnLastStableStates[btnIndex] = LOW;
            isFallEdge = true;
        }
        // Check for rising edge (button release)
        if (btnReading == HIGH && btnLastStableStates[btnIndex] == LOW) {
            btnLastStableStates[btnIndex] = HIGH;
            isRiseEdge = true;
        }
    }
    
    // Always update the last reading
    btnLastStates[btnIndex] = btnReading;
    
    return riseEdgeMode ? isRiseEdge : isFallEdge;
}

// ============================================================================
// TASK INSTANCES
// ============================================================================
Task task1;  // Button and LED toggle
Task task2;  // Blinking LED
Task task3;  // Counter with two buttons
Task taskIdle; // Reporting/monitoring task

// ============================================================================
// TASK IMPLEMENTATIONS
// ============================================================================

/**
 * Task 1: Button and LED Toggle
 * - Detects button press on BTN1_PIN
 * - Toggles LED1 state
 * - Provider: updates led1_state
 */
void executeTask1() {
    // Read button with debouncing
    bool fallingEdge = debounceButton(BTN1_PIN);

    if (fallingEdge) {
        // Button pressed (falling edge)
        led1_state = !led1_state;
        digitalWrite(LED1_PIN, led1_state ? HIGH : LOW);
    }
}

/**
 * Task 2: Blinking LED
 * - Blinks LED2 only when LED1 is OFF
 * - Provider: updates led2_state, led2_onTime, led2_offTime
 * - Consumer: reads led1_state
 */
void executeTask2() {
    // Only blink if LED1 is OFF
    if (!led1_state) {
        // Toggle LED2
        led2_state = !led2_state;
        digitalWrite(LED2_PIN, led2_state ? HIGH : LOW);
        
        // Track time in current state
        unsigned long currentTime = millis();
        if (led2_state) {
            led2_offTime += (currentTime - led2_lastChange);
        } else {
            led2_onTime += (currentTime - led2_lastChange);
        }
        led2_lastChange = currentTime;
    } else {
        // LED1 is ON, turn off LED2
        if (led2_state) {
            unsigned long currentTime = millis();
            led2_onTime += (currentTime - led2_lastChange);
            led2_lastChange = currentTime;
        }
        led2_state = false;
        digitalWrite(LED2_PIN, LOW);
    }
}

/**
 * Task 3: Counter Control
 * - BTN2 increments counter
 * - BTN3 decrements counter
 * - Provider: updates counter
 * - Consumer: could use led2_state information
 */
void executeTask3() {
    // Read BTN2 (increment)
    bool btn2Reading = debounceButton(BTN2_PIN);
    if (btn2Reading) {
        counter++;
    }
    
    // Read BTN3 (decrement)
    bool btn3Reading = debounceButton(BTN3_PIN);
    if (btn3Reading) {
        counter--;
    }
}

/**
 * Idle Task: Reporting and Monitoring
 * - Consumer: reads all global state variables
 * - Reports system state via Serial (STDIO)
 */
void executeIdleTask() {
    // Use \r\n for proper carriage return in Serial monitor
    printf("\r\n=== Lab 2.1 Status ===\r\n");
    printf("LED1: %s\r\n", led1_state ? "ON " : "OFF");
    printf("LED2: %s\r\n", led2_state ? "ON " : "OFF");
    printf("Counter: %d\r\n", counter);
    printf("LED2 ON:  %lu ms\r\n", led2_onTime);
    printf("LED2 OFF: %lu ms\r\n", led2_offTime);
    printf("Uptime: %lu ms\r\n", millis());
    printf("=====================\r\n\r\n");
}

// ============================================================================
// SCHEDULER
// ============================================================================

/**
 * Checks if a task should run based on its period and offset
 */
bool shouldRunTask(Task* task) {
    unsigned long currentTime = millis();
    
    // First run check
    if (task->firstRun) {
        if (currentTime >= task->offset) {
            task->firstRun = false;
            task->lastRun = currentTime;
            return true;
        }
        return false;
    }
    
    // Periodic execution
    if (currentTime - task->lastRun >= task->period) {
        task->lastRun = currentTime;
        return true;
    }
    
    return false;
}

// ============================================================================
// ARDUINO SETUP AND LOOP
// ============================================================================

void setup() {
    // Initialize Serial for STDIO
    initSerialStdio(SERIAL_BAUD_RATE);
    
    printf("Lab 2.1 - Sequential Task Scheduler\r\n");
    printf("Initializing...\r\n");
    
    // Configure pins
    pinMode(BTN1_PIN, INPUT_PULLUP);
    pinMode(BTN2_PIN, INPUT_PULLUP);
    pinMode(BTN3_PIN, INPUT_PULLUP);
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    
    // Initialize LEDs to OFF
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    
    // Initialize task parameters
    // Task 1: Check button every 20ms (50Hz for responsive input)
    task1.period = 20;
    task1.offset = 0;
    task1.lastRun = 0;
    task1.firstRun = true;
    
    // Task 2: Blink every 500ms (2Hz)
    task2.period = 500;
    task2.offset = 10;  // Small offset to avoid collision
    task2.lastRun = 0;
    task2.firstRun = true;
    led2_lastChange = millis();
    
    // Task 3: Check buttons every 30ms
    task3.period = 30;
    task3.offset = 20;  // Different offset
    task3.lastRun = 0;
    task3.firstRun = true;
    
    // Idle Task: Report every 1000ms (1Hz)
    taskIdle.period = 1000;
    taskIdle.offset = 100;  // Start after other tasks
    taskIdle.lastRun = 0;
    taskIdle.firstRun = true;
    
    printf("Initialization complete!\r\n");
    printf("BTN1 (pin %d): Toggle LED1\r\n", BTN1_PIN);
    printf("BTN2 (pin %d): Increment counter\r\n", BTN2_PIN);
    printf("BTN3 (pin %d): Decrement counter\r\n", BTN3_PIN);
    delay(500);
}

void loop() {
    // Sequential (non-preemptive) task execution
    // Each task runs to completion before the next one
    
    // Task 1: Button and LED
    if (shouldRunTask(&task1)) {
        executeTask1();

    }
    
    // Task 2: Blinking LED
    if (shouldRunTask(&task2)) {
        executeTask2();
    }
    
    // Task 3: Counter
    if (shouldRunTask(&task3)) {
        executeTask3();
    }
    
    // Idle Task: Reporting
    if (shouldRunTask(&taskIdle)) {
        executeIdleTask();
    }
    
    // Small delay to prevent excessive CPU usage
    // This is the "idle" time when no tasks are running
    delay(1);
}

#endif

