#ifdef LAB_4_2

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <stdio.h>
#include <Wire.h>
#include <semphr.h>
#include <stdlib.h>

#include "config.h"
#include "serial_stdio.h"
#include "my_a4988.h"
#include "lcd_stdio.h"
#include "command_handler.h"

// -----------------------------------------------------------------------------
// Hardware configuration
// -----------------------------------------------------------------------------
constexpr uint8_t STEP_PIN = 3;
constexpr uint8_t DIR_PIN = 4;
constexpr uint8_t ENABLE_PIN = 5;
constexpr uint8_t STATUS_LED_PIN = 13;

constexpr uint8_t LCD_I2C_ADDRESS = 0x27;
constexpr uint8_t LCD_COLUMNS = 16;
constexpr uint8_t LCD_ROWS = 2;

constexpr TickType_t STATUS_UPDATE_PERIOD = pdMS_TO_TICKS(500);
constexpr TickType_t LED_BLINK_PERIOD = pdMS_TO_TICKS(1000);

// -----------------------------------------------------------------------------
// Shared data structures (protected by mutex)
// -----------------------------------------------------------------------------
struct MotorState {
    int8_t power;        // Current power (-100 to +100)
    bool isRunning;      // Motor is running (power != 0)
    const char* direction; // "FWD" or "REV" or "STOP"
};

static A4988Motor gMotor;
static CommandHandler gCommandHandler;
static FILE gLcdStream;
static MotorState gMotorState{0, false, "STOP"};
static SemaphoreHandle_t gStateMutex = nullptr;

// -----------------------------------------------------------------------------
// Helper functions for thread-safe access
// -----------------------------------------------------------------------------
static int lcdStreamPutchar(char c, FILE *file) {
    return LCDStdio::putcharlcd(c, file);
}

static MotorState getMotorStateSnapshot() {
    MotorState snapshot{0, false, "STOP"};
    if (gStateMutex != nullptr && xSemaphoreTake(gStateMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        snapshot = gMotorState;
        xSemaphoreGive(gStateMutex);
    }
    return snapshot;
}

static void updateMotorState(int8_t power) {
    if (gStateMutex != nullptr && xSemaphoreTake(gStateMutex, portMAX_DELAY) == pdTRUE) {
        gMotorState.power = power;
        gMotorState.isRunning = (power != 0);
        if (power > 0) {
            gMotorState.direction = "FWD";
        } else if (power < 0) {
            gMotorState.direction = "REV";
        } else {
            gMotorState.direction = "STOP";
        }
        xSemaphoreGive(gStateMutex);
    }
}

static void updateStatusDisplay() {
    MotorState state = getMotorStateSnapshot();
    fprintf(&gLcdStream,
            "\fMotor: %s\nPower: %+4d%%",
            state.direction,
            state.power);
}

// -----------------------------------------------------------------------------
// Command callbacks for unified handler
// -----------------------------------------------------------------------------
static bool cmdMotorSet(void* context, const char* args) {
    (void) context;
    
    if (args == nullptr || *args == '\0') {
        printf("\fError: motor set requires value [-100..100]\r\n");
        return false;
    }
    
    // Parse power value
    int power = atoi(args);
    if (power < -100 || power > 100) {
        printf("\fError: Power must be [-100..100]\r\n");
        return false;
    }
    
    a4988_set_power(&gMotor, static_cast<int8_t>(power));
    updateMotorState(power);
    printf("\fMotor set to %+d%%\r\n", power);
    return true;
}

static bool cmdMotorStop(void* context, const char* args) {
    (void) context;
    (void) args;
    a4988_stop(&gMotor);
    updateMotorState(0);
    printf("\fMotor stopped\r\n");
    return true;
}

static bool cmdMotorMax(void* context, const char* args) {
    (void) context;
    (void) args;
    MotorState state = getMotorStateSnapshot();
    int8_t maxPower = (state.power >= 0) ? 100 : -100;
    a4988_set_max(&gMotor);
    updateMotorState(maxPower);
    printf("\fMotor set to maximum (%+d%%)\r\n", maxPower);
    return true;
}

static bool cmdMotorInc(void* context, const char* args) {
    (void) context;
    (void) args;
    MotorState state = getMotorStateSnapshot();
    int8_t oldPower = state.power;
    a4988_increase_power(&gMotor, 10);
    int8_t newPower = a4988_get_power(&gMotor);
    updateMotorState(newPower);
    printf("\fMotor power increased: %+d%% -> %+d%%\r\n", oldPower, newPower);
    return true;
}

static bool cmdMotorDec(void* context, const char* args) {
    (void) context;
    (void) args;
    MotorState state = getMotorStateSnapshot();
    int8_t oldPower = state.power;
    a4988_decrease_power(&gMotor, 10);
    int8_t newPower = a4988_get_power(&gMotor);
    updateMotorState(newPower);
    printf("\fMotor power decreased: %+d%% -> %+d%%\r\n", oldPower, newPower);
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
    commandHandlerPrintHelp(&gCommandHandler);
    return false;
}

// -----------------------------------------------------------------------------
// FreeRTOS Task declarations
// -----------------------------------------------------------------------------
void TaskCommandProcessor(void *pvParameters);
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
// Task 2: Status Display (LCD and Serial updates)
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
// Task 3: Status LED Blink (System heartbeat)
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

    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, LOW);

    // Initialize A4988 motor driver
    a4988_init(&gMotor, STEP_PIN, DIR_PIN, ENABLE_PIN);
    a4988_stop(&gMotor);
    updateMotorState(0);
    
    // Start interrupt-based motor control
    a4988_start_interrupts();

    Wire.begin();
    LCDStdio::init(LCD_I2C_ADDRESS, LCD_COLUMNS, LCD_ROWS);
    LCDStdio::clear();
    fdev_setup_stream(&gLcdStream, lcdStreamPutchar, nullptr, _FDEV_SETUP_WRITE);

    gStateMutex = xSemaphoreCreateMutex();

    // Initialize unified command handler with default callback for unknown commands
    commandHandlerInit(&gCommandHandler, cmdUnknown, nullptr);

    // Register commands dynamically
    commandHandlerRegister(&gCommandHandler, "motor set", cmdMotorSet, nullptr, "Set motor power [-100..100]");
    commandHandlerRegister(&gCommandHandler, "motor stop", cmdMotorStop, nullptr, "Stop motor immediately");
    commandHandlerRegister(&gCommandHandler, "motor max", cmdMotorMax, nullptr, "Set motor to maximum power");
    commandHandlerRegister(&gCommandHandler, "motor inc", cmdMotorInc, nullptr, "Increase power by 10%");
    commandHandlerRegister(&gCommandHandler, "motor dec", cmdMotorDec, nullptr, "Decrease power by 10%");
    commandHandlerRegister(&gCommandHandler, "status", cmdStatus, nullptr, "Show current status");
    commandHandlerRegister(&gCommandHandler, "help", cmdHelp, nullptr, "Show help");

    fprintf(&gLcdStream, "\fLab 4.2 Ready\nInit FreeRTOS...");
    printf("Lab 4.2: Stepper Motor Control System Ready\r\n");
    printf("Type 'help' for available commands\r\n");
    printf("Commands: motor set [-100..100], motor stop, motor max, motor inc, motor dec\r\n");

    // Create FreeRTOS tasks
    // Motor control is now interrupt-based, so no dedicated task needed
    // Priority order: CommandProcessor (2) > StatusDisplay (1) > StatusLED (0)
    xTaskCreate(TaskCommandProcessor, "CmdProc", 256, nullptr, 2, nullptr);
    xTaskCreate(TaskStatusDisplay, "StatusDisp", 256, nullptr, 1, nullptr);
    xTaskCreate(TaskStatusLED, "StatusLED", 128, nullptr, 0, nullptr);

    printf("FreeRTOS scheduler starting...\r\n");
    fprintf(&gLcdStream, "\fLab 4.2 Ready\nFreeRTOS active");

    vTaskStartScheduler();

    // Should never reach here
    while (true) {
        // Error handler
    }
}

void loop() {
    // Not used - FreeRTOS scheduler handles execution
}

#endif  // LAB_4_2
