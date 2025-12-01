#ifdef LAB_6_1

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "fsm.h"
#include "rtos_btn.h"

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
// Pin definitions (from diagram.json)
constexpr uint8_t RED_LED_PIN = 5;
constexpr uint8_t GREEN_LED_PIN = 4;
constexpr uint8_t BUTTON_PIN = 15;
constexpr uint8_t STATUS_LED_PIN = 13;

constexpr TickType_t FSM_UPDATE_PERIOD = pdMS_TO_TICKS(50);
constexpr TickType_t STATUS_LED_BLINK_PERIOD = pdMS_TO_TICKS(1000);

// -----------------------------------------------------------------------------
// State IDs
// -----------------------------------------------------------------------------
enum LedStates {
    STATE_RED_LED = 0,
    STATE_GREEN_LED = 1
};

// Event IDs
enum Events {
    EVENT_BUTTON_PRESS = 1
};

// -----------------------------------------------------------------------------
// Global objects
// -----------------------------------------------------------------------------
static FSM gLedFsm;
static RTOSButton gButton(BUTTON_PIN, true);  // Pullup enabled
static SemaphoreHandle_t gFsmMutex = nullptr;

// -----------------------------------------------------------------------------
// FSM State callbacks
// -----------------------------------------------------------------------------
// State callback: Red LED active
void state_red_led_enter(FSM* fsm) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println("[FSM] State: RED LED ON");
}

// State callback: Green LED active
void state_green_led_enter(FSM* fsm) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
    Serial.println("[FSM] State: GREEN LED ON");
}

// -----------------------------------------------------------------------------
// FreeRTOS Task declarations
// -----------------------------------------------------------------------------
void TaskFsmProcessor(void *pvParameters);
void TaskStatusLED(void *pvParameters);

// -----------------------------------------------------------------------------
// Task 1: FSM Processor (monitors button and processes state transitions)
// -----------------------------------------------------------------------------
void TaskFsmProcessor(void *pvParameters) {
    (void) pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    Serial.println("[TaskFsmProcessor] Started.");
    for (;;) {
        // Check for button press (thread-safe)
        if (gButton.consumePress()) {
            // Take mutex to protect FSM state
            if (xSemaphoreTake(gFsmMutex, portMAX_DELAY) == pdTRUE) {
                fsm_process_event(&gLedFsm, EVENT_BUTTON_PRESS);
                xSemaphoreGive(gFsmMutex);
            }
        }
        
        // Update FSM
        if (xSemaphoreTake(gFsmMutex, portMAX_DELAY) == pdTRUE) {
            fsm_update(&gLedFsm);
            xSemaphoreGive(gFsmMutex);
        }
        
        vTaskDelayUntil(&lastWakeTime, FSM_UPDATE_PERIOD);
    }
}

// -----------------------------------------------------------------------------
// Task 2: Status LED Blink (System heartbeat)
// -----------------------------------------------------------------------------
void TaskStatusLED(void *pvParameters) {
    (void) pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    bool ledState = false;
    
    for (;;) {
        ledState = !ledState;
        digitalWrite(STATUS_LED_PIN, ledState ? HIGH : LOW);
        vTaskDelayUntil(&lastWakeTime, STATUS_LED_BLINK_PERIOD);
    }
}

// -----------------------------------------------------------------------------
// Arduino setup & loop
// -----------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);

    // Configure LED pins
    pinMode(RED_LED_PIN, OUTPUT);
    pinMode(GREEN_LED_PIN, OUTPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(STATUS_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    Serial.println("\n=== Lab 6.1: LED State Machine (FreeRTOS) ===");
    
    // Create FSM mutex
    gFsmMutex = xSemaphoreCreateMutex();
    if (gFsmMutex == nullptr) {
        Serial.println("ERROR: Failed to create FSM mutex!");
        while (1);  // Halt
    }
    
    // Initialize FSM
    fsm_init(&gLedFsm, "LED_FSM");
    
    // Add states
    fsm_add_state(&gLedFsm, STATE_RED_LED, "RED_LED", 
                  state_red_led_enter, NULL, NULL);
    fsm_add_state(&gLedFsm, STATE_GREEN_LED, "GREEN_LED", 
                  state_green_led_enter, NULL, NULL);
    
    // Add transitions (toggle between states on button press)
    fsm_add_transition(&gLedFsm, STATE_RED_LED, STATE_GREEN_LED, 
                       EVENT_BUTTON_PRESS, NULL, NULL);
    fsm_add_transition(&gLedFsm, STATE_GREEN_LED, STATE_RED_LED, 
                       EVENT_BUTTON_PRESS, NULL, NULL);
    
    // Start FSM in RED_LED state
    fsm_start(&gLedFsm, STATE_RED_LED);
    
    Serial.println("FSM initialized. Press button to toggle LEDs.");
    
    // Start RTOS button monitoring
    if (!gButton.start(pdMS_TO_TICKS(10), 3)) {
        Serial.println("ERROR: Failed to start button monitoring!");
        while (1);  // Halt
    }
    
    Serial.println("Button monitoring started.");
    
    // Create FreeRTOS tasks
    // Priority order: ButtonMonitor (3) > FsmProcessor (2) > StatusLED (1)
    xTaskCreate(TaskFsmProcessor, "FsmProc", 256, nullptr, 3, nullptr);
    xTaskCreate(TaskStatusLED, "StatusLED", 128, nullptr, 1, nullptr);
    
    Serial.println("FreeRTOS scheduler starting...");
    
    // Start scheduler
    vTaskStartScheduler();
    
    // Should never reach here
    Serial.println("ERROR: Scheduler failed to start!");
    while (1) {
        // Error handler
    }
}

void loop() {
    // Not used - FreeRTOS scheduler handles execution
}

#endif // LAB_6_1

