#ifdef LAB_1_1
#include <Arduino.h>
#include "config.h"
#include "my_led.h"
#include "interactive_scanf.h"
#include "serial_stdio.h"  // Add this include

#define LED_PIN DEFAULT_LED_PIN
#define BUTTON_PIN DEFAULT_BUTTON_PIN

#define LED_ON_COMMAND "led_on"
#define LED_OFF_COMMAND "led_off"
#define LED_TOGGLE_COMMAND "led_toggle"
#define EXIT_COMMAND "exit"
#define IS_LED_ON_COMMAND "status"

#define INTERACTIVE_SCANF_USE

// Lab 1.1: Control an LED via serial commands

LedUtils led(LED_PIN);
void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
    initSerialStdio(SERIAL_BAUD_RATE);  // Replace Serial.begin() with this
    printf("\nLab 1.1 - LED Control via Serial");
    printf("\nCommands:");
    printf("\n  '" LED_ON_COMMAND  "' - turn LED ON");
    printf("\n  '" LED_OFF_COMMAND "' - turn LED OFF");
    printf("\n  '" LED_TOGGLE_COMMAND "' - toggle LED state");
    printf("\n  '" IS_LED_ON_COMMAND "' - check LED status");
    printf("\n  '" EXIT_COMMAND "' - restart the board");
    printf("\nReady for commands...\n");
}

void loop() {
    delay(10); //Debounce time: 10 ms (to avoid busy loop)

    char buffer[32];

    #ifdef INTERACTIVE_SCANF_USE
        interactiveScanf(buffer, 32, "\nEnter command: ", "", "", "\r\n");
    #else
        scanf("%31s", buffer);
    #endif
    
    if (strcmp(buffer, LED_ON_COMMAND) == 0) {
        led.turnOn();
        printf("\nLED turned ON\n");
    }
    else if (strcmp(buffer, LED_OFF_COMMAND) == 0) {
        led.turnOff();
        printf("\nLED turned OFF\n");
    }
    else if (strcmp(buffer, LED_TOGGLE_COMMAND) == 0) {
        led.toggle();
    }
    else if (strcmp(buffer, IS_LED_ON_COMMAND) == 0) {
        if (led.isOn()) {
            printf("\nLED is currently ON\n");
        } else {
            printf("\nLED is currently OFF\n");
        }
    }
    else if (strcmp(buffer, EXIT_COMMAND) == 0) {
        printf("\nRestarting board...\n");
        delay(1000); // give time to send message
        resetFunc(); // call reset
    }
    else {
        printf("\nUnknown command: %s", buffer);
        printf("\nUse commands:");
        printf("\n  '" LED_ON_COMMAND "'");
        printf("\n  '" LED_OFF_COMMAND "'");
        printf("\n  '" LED_TOGGLE_COMMAND "'");
        printf("\n  '" IS_LED_ON_COMMAND "'");
        printf("\n  '" EXIT_COMMAND "'\n");
    }
    
}
#endif