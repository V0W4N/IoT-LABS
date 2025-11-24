#ifdef LAB_4_1

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>
#include <Wire.h>
#include <semphr.h>

#include "config.h"
#include "serial_stdio.h"
#include "my_relay.h"
#include "lcd_stdio.h"
#include "command_handler.h"

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
constexpr uint8_t RELAY_PIN = 2;
constexpr uint8_t POTENTIOMETER_PIN = A15;
constexpr uint8_t STATUS_LED_PIN = 13;

constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;

constexpr uint16_t POT_THRESHOLD = 716;  // ~70% of ADC range
constexpr uint16_t ADC_RESOLUTION = 1023;

constexpr TickType_t POT_CHECK_PERIOD = pdMS_TO_TICKS(100);
constexpr TickType_t STATUS_UPDATE_PERIOD = pdMS_TO_TICKS(500);
constexpr TickType_t LED_BLINK_PERIOD = pdMS_TO_TICKS(1000);

// -----------------------------------------------------------------------------
// Shared data structures (protected by mutex)
// -----------------------------------------------------------------------------
struct RelayState {
    bool relayOn;
    bool autoMode;
    uint16_t potValue;
    uint8_t potPercent;
};

static Relay gRelay;
static CommandHandler gCommandHandler;
static FILE gLcdStream;
static RelayState gRelayState{false, false, 0, 0};
static SemaphoreHandle_t gStateMutex = nullptr;

// -----------------------------------------------------------------------------
// Helper functions for thread-safe access
// -----------------------------------------------------------------------------
static int lcdStreamPutchar(char c, FILE *file) {
    return LCDStdio::putcharlcd(c, file);
}

static RelayState getRelayStateSnapshot() {
    RelayState snapshot{false, false, 0, 0};
    if (gStateMutex != nullptr && xSemaphoreTake(gStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        snapshot = gRelayState;
        xSemaphoreGive(gStateMutex);
    }
    return snapshot;
}

static void updateRelayState(bool relayOn, bool autoMode, uint16_t potValue, uint8_t potPercent) {
    if (gStateMutex != nullptr && xSemaphoreTake(gStateMutex, portMAX_DELAY) == pdTRUE) {
        gRelayState.relayOn = relayOn;
        gRelayState.autoMode = autoMode;
        gRelayState.potValue = potValue;
        gRelayState.potPercent = potPercent;
        xSemaphoreGive(gStateMutex);
    }
}

static void setAutoMode(bool autoMode) {
    if (gStateMutex != nullptr && xSemaphoreTake(gStateMutex, portMAX_DELAY) == pdTRUE) {
        gRelayState.autoMode = autoMode;
        xSemaphoreGive(gStateMutex);
    }
}

static void updateStatusDisplay() {
    RelayState state = getRelayStateSnapshot();
    fprintf(&gLcdStream,
            "\fRelay: %s\nPot: %3u%% (%s)",
            state.relayOn ? "ON " : "OFF",
            state.potPercent,
            state.autoMode ? "AUTO" : "MAN");
}

// -----------------------------------------------------------------------------
// Command callbacks for unified handler
// -----------------------------------------------------------------------------
static bool cmdRelayOn(void* context, const char* args) {
    (void) context;
    (void) args;
    RelayState state = getRelayStateSnapshot();
    relay_turn_on(&gRelay);
    setAutoMode(false);
    updateRelayState(true, false, state.potValue, state.potPercent);
    printf("\fRelay: ON\r\n");
    fprintf(&gLcdStream, "\fRelay: ON\nManual mode");
    return true;
}

static bool cmdRelayOff(void* context, const char* args) {
    (void) context;
    (void) args;
    RelayState state = getRelayStateSnapshot();
    relay_turn_off(&gRelay);
    setAutoMode(false);
    updateRelayState(false, false, state.potValue, state.potPercent);
    printf("\fRelay: OFF\r\n");
    fprintf(&gLcdStream, "\fRelay: OFF\nManual mode");
    return true;
}

static bool cmdStatus(void* context, const char* args) {
    (void) context;
    (void) args;
    updateStatusDisplay();
    return true;
}

static bool cmdHelp(void* context, const char* args) {
    (void) context;
    (void) args;
    commandHandlerPrintHelp(&gCommandHandler);
    return true;
}

static bool cmdUnknown(void* context, const char* command) {
    (void) context;
    printf("\fUnknown command: %s\r\n", command ? command : "");
    fprintf(&gLcdStream, "\fUnknown cmd\nTry: relay on/off");
    commandHandlerPrintHelp(&gCommandHandler);
    return false;
}

// -----------------------------------------------------------------------------
// FreeRTOS Task declarations
// -----------------------------------------------------------------------------
void TaskCommandProcessor(void *pvParameters);
void TaskPotentiometerMonitor(void *pvParameters);
void TaskStatusDisplay(void *pvParameters);
void TaskStatusLED(void *pvParameters);

// -----------------------------------------------------------------------------
// Task 1: Command Processor (STDIO input)
// -----------------------------------------------------------------------------
void TaskCommandProcessor(void *pvParameters) {
    (void) pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(50);

    for (;;) {
        if (Serial.available()) {
            int c = getchar();
            if (c != EOF && c != '\0') {
                commandHandlerProcessChar(&gCommandHandler, static_cast<char>(c));
            }
        }
        vTaskDelayUntil(&lastWakeTime, xFrequency);
    }
}

// -----------------------------------------------------------------------------
// Task 2: Potentiometer Monitor (Auto relay control)
// -----------------------------------------------------------------------------
void TaskPotentiometerMonitor(void *pvParameters) {
    (void) pvParameters;
    TickType_t lastWakeTime = xTaskGetTickCount();

    for (;;) {
        uint16_t potValue = analogRead(POTENTIOMETER_PIN);
        uint8_t potPercent = (potValue * 100) / ADC_RESOLUTION;
        RelayState state = getRelayStateSnapshot();

        // Auto mode: activate relay when pot > 70%
        if (potValue >= POT_THRESHOLD) {
            if (!relay_is_on(&gRelay)) {
                relay_turn_on(&gRelay);
                setAutoMode(true);
                updateRelayState(true, true, potValue, potPercent);
            } else if (state.autoMode) {
                updateRelayState(true, true, potValue, potPercent);
            }
        } else {
            if (relay_is_on(&gRelay) && state.autoMode) {
                relay_turn_off(&gRelay);
                updateRelayState(false, true, potValue, potPercent);
                printf("[Auto] Relay OFF (Pot: %u%%)\r\n", potPercent);
            } else {
                updateRelayState(state.relayOn, state.autoMode, potValue, potPercent);
            }
        }
        vTaskDelayUntil(&lastWakeTime, POT_CHECK_PERIOD);
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
    initSerialStdio(SERIAL_BAUD_RATE);

    pinMode(POTENTIOMETER_PIN, INPUT);
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    relay_init(&gRelay, RELAY_PIN);
    relay_turn_off(&gRelay);

    Wire.begin();
    LCDStdio::init(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
    LCDStdio::clear();
    fdev_setup_stream(&gLcdStream, lcdStreamPutchar, nullptr, _FDEV_SETUP_WRITE);

    gStateMutex = xSemaphoreCreateMutex();

    // Initialize unified command handler with default callback for unknown commands
    commandHandlerInit(&gCommandHandler, cmdUnknown, nullptr);

    // Register commands dynamically
    commandHandlerRegister(&gCommandHandler, "relay on", cmdRelayOn, nullptr, "Turn relay ON");
    commandHandlerRegister(&gCommandHandler, "relay off", cmdRelayOff, nullptr, "Turn relay OFF");
    commandHandlerRegister(&gCommandHandler, "status", cmdStatus, nullptr, "Show current status");
    commandHandlerRegister(&gCommandHandler, "help", cmdHelp, nullptr, "Show help");

    fprintf(&gLcdStream, "\fLab 4.1 Ready\nInit FreeRTOS...");
    printf("Lab 4.1: Relay Control System Ready\r\n");
    printf("Type 'help' for available commands\r\n");
    printf("Auto mode: Relay activates at pot > 70%%\r\n");

    // Create FreeRTOS tasks
    // Priority order: PotMonitor (3) > CommandProcessor (2) > StatusDisplay (1) > StatusLED (0)
    xTaskCreate(TaskPotentiometerMonitor, "PotMonitor", 256, nullptr, 3, nullptr);
    xTaskCreate(TaskCommandProcessor, "CmdProc", 256, nullptr, 2, nullptr);
    xTaskCreate(TaskStatusDisplay, "StatusDisp", 256, nullptr, 1, nullptr);
    xTaskCreate(TaskStatusLED, "StatusLED", 128, nullptr, 0, nullptr);

    printf("FreeRTOS scheduler starting...\r\n");
    fprintf(&gLcdStream, "\fLab 4.1 Ready\nFreeRTOS active");

    vTaskStartScheduler();

    // Should never reach here
    while (true) {
        // Error handler
    }
}

void loop() {
    // Not used - FreeRTOS scheduler handles execution
}

#endif  // LAB_4_1
