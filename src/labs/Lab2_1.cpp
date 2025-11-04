#ifdef LAB_2_1
#include <Arduino.h>
#include "config.h"
#include "serial_stdio.h"

// Lab 2.1 - Interrupt-Driven Task Execution with Provider/Consumer Model
// Using Timer1 interrupt for scheduler and External/Pin Change interrupts for buttons

// ============================================================================
// HARDWARE CONFIGURATION
// ============================================================================
#define BTN1_PIN 20         // Button for Task 1 (toggle LED1) - INT4
#define BTN2_PIN 19         // Button for Task 3 (increment counter) - INT5
#define BTN3_PIN 18         // Button for Task 3 (decrement counter) - INT0
#define LED1_PIN 13         // LED for Task 1 (toggle on button press)
#define LED2_PIN 12         // LED for Task 2 (blink when LED1 is OFF)


// ============================================================================
// TASK STRUCTURE
// ============================================================================
typedef void (*TaskFunction)(void);  // Function pointer type for task execution

struct Task {
    const char* name;          // Task name for debugging
    unsigned long period;      // Period in milliseconds (recurrence)
    unsigned long offset;      // Offset from start (initial delay)
    volatile unsigned long lastRun;     // Last execution time
    volatile bool needsExecution;       // Flag set by timer interrupt
    bool firstRun;             // Flag for first execution
    TaskFunction execute;      // Function to execute
};

// ============================================================================
// GLOBAL VARIABLES (Provider/Consumer Model)
// ============================================================================
// Task 1 state (Provider)
volatile bool led1_state = false;           // Current state of LED1

// Task 2 state (Provider)
volatile bool led2_state = false;           // Current state of LED2
volatile unsigned long led2_onTime = 0;     // Time LED2 has been ON (ms)
volatile unsigned long led2_offTime = 0;    // Time LED2 has been OFF (ms)
volatile unsigned long led2_lastChange = 0; // Last time LED2 changed state

// Task 3 state (Provider)
volatile int counter = 0;                   // Counter value

// Button interrupt flags
volatile bool btn1_pressed = false;
volatile bool btn2_pressed = false;
volatile bool btn3_pressed = false;

// Timing
volatile unsigned long systemTicks = 0;     // 1ms tick counter

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================
void executeTask1();
void executeTask2();
void executeTask3();
void executeIdleTask();

// ============================================================================
// TASK LIST
// ============================================================================
#define NUM_TASKS 4

Task taskList[NUM_TASKS] = {
    {"Task1_Button",  20,   0,   0, false, true, executeTask1},    // Check button every 20ms
    {"Task2_Blink",   500,  10,  0, false, true, executeTask2},    // Blink every 500ms
    {"Task3_Counter", 30,   20,  0, false, true, executeTask3},    // Check buttons every 30ms
    {"TaskIdle",      1000, 100, 0, false, true, executeIdleTask}  // Report every 1000ms
};

// ============================================================================
// TIMER1 INTERRUPT - 1ms tick for scheduler
// ============================================================================
ISR(TIMER1_COMPA_vect) {
    systemTicks++;
    unsigned long currentTime = systemTicks;
    
    // Check all tasks in the list
    for (uint8_t i = 0; i < NUM_TASKS; i++) {
        Task* task = &taskList[i];
        
        // First run check
        if (task->firstRun && currentTime >= task->offset) {
            task->needsExecution = true;
        }
        // Periodic execution check
        else if (!task->firstRun && (currentTime - task->lastRun >= task->period)) {
            task->needsExecution = true;
        }
    }
}

// ============================================================================
// INTERRUPT SERVICE ROUTINES (ISRs)
// ============================================================================

void btn1_ISR() {
    btn1_pressed = true;
}

void btn2_ISR() {
    btn2_pressed = true;
}

void btn3_ISR() {
    btn3_pressed = true;
}

// ============================================================================
// TASK IMPLEMENTATIONS
// ============================================================================

/**
 * Task 1: Button and LED Toggle
 * - Detects button press on BTN1_PIN via interrupt flag
 * - Toggles LED1 state
 * - Provider: updates led1_state
 */
void executeTask1() {
    static bool btnLastState = HIGH;
    static unsigned long btnLastDebounce = 0;
    const unsigned long DEBOUNCE_DELAY = 50;
    
    // Check interrupt flag
    bool btnPressed = false;
    noInterrupts();
    if (btn1_pressed) {
        btn1_pressed = false;
        btnPressed = true;
    }
    interrupts();
    
    // Debounce logic
    if (btnPressed) {
        unsigned long currentTime = systemTicks;
        if ((currentTime - btnLastDebounce) > DEBOUNCE_DELAY) {
            if (btnLastState == HIGH) {
                // Toggle LED1
                led1_state = !led1_state;
                digitalWrite(LED1_PIN, led1_state ? HIGH : LOW);
                btnLastState = LOW;
            }
            btnLastDebounce = currentTime;
        }
    } else {
        // Check if button released
        if (digitalRead(BTN1_PIN) == HIGH) {
            btnLastState = HIGH;
        }
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
        unsigned long currentTime = systemTicks;
        if (led2_state) {
            led2_offTime += (currentTime - led2_lastChange);
        } else {
            led2_onTime += (currentTime - led2_lastChange);
        }
        led2_lastChange = currentTime;
    } else {
        // LED1 is ON, turn off LED2
        if (led2_state) {
            unsigned long currentTime = systemTicks;
            led2_onTime += (currentTime - led2_lastChange);
            led2_lastChange = currentTime;
        }
        led2_state = false;
        digitalWrite(LED2_PIN, LOW);
    }
}

/**
 * Task 3: Counter Control
 * - BTN2 increments counter (via interrupt flag)
 * - BTN3 decrements counter (via interrupt flag)
 * - Provider: updates counter
 */
void executeTask3() {
    static bool btn2LastState = HIGH;
    static bool btn3LastState = HIGH;
    static unsigned long btn2LastDebounce = 0;
    static unsigned long btn3LastDebounce = 0;
    const unsigned long DEBOUNCE_DELAY = 50;
    
    unsigned long currentTime = systemTicks;
    
    // Handle BTN2 (increment)
    bool btn2Pressed = false;
    noInterrupts();
    if (btn2_pressed) {
        btn2_pressed = false;
        btn2Pressed = true;
    }
    interrupts();
    
    if (btn2Pressed) {
        if ((currentTime - btn2LastDebounce) > DEBOUNCE_DELAY) {
            if (btn2LastState == HIGH) {
                counter++;
                btn2LastState = LOW;
            }
            btn2LastDebounce = currentTime;
        }
    } else {
        if (digitalRead(BTN2_PIN) == HIGH) {
            btn2LastState = HIGH;
        }
    }
    
    // Handle BTN3 (decrement)
    bool btn3Pressed = false;
    noInterrupts();
    if (btn3_pressed) {
        btn3_pressed = false;
        btn3Pressed = true;
    }
    interrupts();
    
    if (btn3Pressed) {
        if ((currentTime - btn3LastDebounce) > DEBOUNCE_DELAY) {
            if (btn3LastState == HIGH) {
                counter--;
                btn3LastState = LOW;
            }
            btn3LastDebounce = currentTime;
        }
    } else {
        if (digitalRead(BTN3_PIN) == HIGH) {
            btn3LastState = HIGH;
        }
    }
}

/**
 * Idle Task: Reporting and Monitoring
 * - Consumer: reads all global state variables
 * - Reports system state via Serial (STDIO)
 */
void executeIdleTask() {
    // Use \r\n for proper carriage return in Serial monitor
    printf("\r\n=== Lab 2.1 Status (Interrupt-Driven) ===\r\n");
    printf("LED1: %s\r\n", led1_state ? "ON " : "OFF");
    printf("LED2: %s\r\n", led2_state ? "ON " : "OFF");
    printf("Counter: %d\r\n", counter);
    printf("LED2 ON:  %lu ms\r\n", led2_onTime);
    printf("LED2 OFF: %lu ms\r\n", led2_offTime);
    printf("Uptime: %lu ms\r\n", systemTicks);
    printf("==========================================\r\n\r\n");
}

// ============================================================================
// INTERRUPT SETUP FUNCTIONS
// ============================================================================

void setupTimer1Interrupt() {
    // Configure Timer1 for 1ms interrupt (1000 Hz)
    // Using CTC mode (Clear Timer on Compare Match)
    
    noInterrupts();  // Disable all interrupts
    
    // Reset Timer1 control registers
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1 = 0;
    
    // Set compare match register for 1ms (1000Hz)
    // For 16MHz clock with prescaler 64:
    // (16,000,000 Hz) / (64 * 1000 Hz) - 1 = 249
    OCR1A = 249;
    
    // Turn on CTC mode
    TCCR1B |= (1 << WGM12);
    
    // Set CS11 and CS10 bits for 64 prescaler
    TCCR1B |= (1 << CS11) | (1 << CS10);
    
    // Enable timer compare interrupt
    TIMSK1 |= (1 << OCIE1A);
    
    interrupts();  // Enable all interrupts
}

void setupExternalInterrupts() {
    // Using attachInterrupt() for cleaner, more portable code
    // Reference: https://docs.arduino.cc/language-reference/en/functions/external-interrupts/attachInterrupt/
    
    // BTN1 (Pin 2) - INT4 on Mega 2560
    // digitalPinToInterrupt() converts pin number to interrupt number
    attachInterrupt(digitalPinToInterrupt(BTN1_PIN), btn1_ISR, FALLING);
    
    // BTN2 (Pin 3) - INT5 on Mega 2560
    attachInterrupt(digitalPinToInterrupt(BTN2_PIN), btn2_ISR, FALLING);
    
    // BTN3 (Pin 21) - INT0 on Mega 2560
    attachInterrupt(digitalPinToInterrupt(BTN3_PIN), btn3_ISR, FALLING);
}

// ============================================================================
// ARDUINO SETUP AND LOOP
// ============================================================================

void setup() {
    // Initialize Serial for STDIO
    initSerialStdio(SERIAL_BAUD_RATE);
    
    printf("Lab 2.1 - Interrupt-Driven Task Scheduler\r\n");
    printf("==========================================\r\n");
    printf("Using Timer1 interrupt for scheduling\r\n");
    printf("Using INT0, INT1, and PCINT for buttons\r\n");
    printf("Initializing...\r\n\r\n");
    
    // Configure pins
    pinMode(BTN1_PIN, INPUT_PULLUP);
    pinMode(BTN2_PIN, INPUT_PULLUP);
    pinMode(BTN3_PIN, INPUT_PULLUP);
    pinMode(LED1_PIN, OUTPUT);
    pinMode(LED2_PIN, OUTPUT);
    
    // Initialize LEDs to OFF
    digitalWrite(LED1_PIN, LOW);
    digitalWrite(LED2_PIN, LOW);
    
    // Initialize LED2 timing
    led2_lastChange = 0;
    
    // Setup hardware interrupts
    setupTimer1Interrupt();
    setupExternalInterrupts();
    
    printf("Initialization complete!\r\n");
    printf("Number of tasks: %d\r\n\r\n", NUM_TASKS);
    
    // Print task information
    for (uint8_t i = 0; i < NUM_TASKS; i++) {
        printf("  [%d] %s: period=%lu ms, offset=%lu ms\r\n", 
               i, taskList[i].name, taskList[i].period, taskList[i].offset);
    }
    
    printf("\r\nHardware Configuration:\r\n");
    printf("BTN1 (pin %d):  Toggle LED1 [attachInterrupt - FALLING]\r\n", BTN1_PIN);
    printf("BTN2 (pin %d):  Increment counter [attachInterrupt - FALLING]\r\n", BTN2_PIN);
    printf("BTN3 (pin %d):  Decrement counter [attachInterrupt - FALLING]\r\n", BTN3_PIN);
    printf("Timer1:         1ms tick for scheduler [ISR]\r\n\r\n");
    
    delay(500);
}

void loop() {
    // Interrupt-driven execution - iterate through task list
    
    for (uint8_t i = 0; i < NUM_TASKS; i++) {
        Task* task = &taskList[i];
        
        // Check if task needs execution
        if (task->needsExecution) {
            // Atomically clear flag and update timing
            noInterrupts();
            task->needsExecution = false;
            task->lastRun = systemTicks;
            task->firstRun = false;
            interrupts();
            
            // Execute the task function
            task->execute();
        }
    }
    
    // CPU can idle here or do other work
    // The loop is no longer polling - it's event-driven by interrupt flags
}

#endif


