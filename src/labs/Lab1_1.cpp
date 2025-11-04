#ifdef LAB_1_1
#include <Arduino.h>
#include "config.h"
#include "my_led.h"
#include "interactive_scanf.h"
#include "serial_stdio.h"  // Add this include

#define LED_PIN 12
#define BUTTON_PIN 2

#define LED_ON_COMMAND "led_on"
#define LED_OFF_COMMAND "led_off"
#define LED_TOGGLE_COMMAND "led_toggle"
#define EXIT_COMMAND "exit"
#define IS_LED_ON_COMMAND "status"


// Lab 1.1: Control an LED via serial commands

Led led;
void(* resetFunc) (void) = 0; //declare reset function @ address 0

void setup() {
    initSerialStdio(SERIAL_BAUD_RATE);  // Replace Serial.begin() with this
    
    // Initialize LED
    led_init(&led, LED_PIN);
    
    printf("\r\nLab 1.1 - LED Control via Serial\r\n");
    printf("Commands:\r\n");
    printf("  '" LED_ON_COMMAND  "' - turn LED ON\r\n");
    printf("  '" LED_OFF_COMMAND "' - turn LED OFF\r\n");
    printf("  '" LED_TOGGLE_COMMAND "' - toggle LED state\r\n");
    printf("  '" IS_LED_ON_COMMAND "' - check LED status\r\n");
    printf("  '" EXIT_COMMAND "' - restart the board\r\n");
    printf("Ready for commands...\r\n");
}

void loop() {
    delay(10); //Debounce time: 10 ms (to avoid busy loop)

    char buffer[32];


    scanf("%31s", buffer);
    
    if (strcmp(buffer, LED_ON_COMMAND) == 0) {
        led_turn_on(&led);
        printf("\r\nLED turned ON\r\n");
    }
    else if (strcmp(buffer, LED_OFF_COMMAND) == 0) {
        led_turn_off(&led);
        printf("\r\nLED turned OFF\r\n");
    }
    else if (strcmp(buffer, LED_TOGGLE_COMMAND) == 0) {
        led_toggle(&led);
        printf("\r\nLED toggled\r\n");
    }
    else if (strcmp(buffer, IS_LED_ON_COMMAND) == 0) {
        if (led_is_on(&led)) {
            printf("\r\nLED is currently ON\r\n");
        } else {
            printf("\r\nLED is currently OFF\r\n");
        }
    }
    else if (strcmp(buffer, EXIT_COMMAND) == 0) {
        printf("\r\nRestarting board...\r\n");
        delay(1000); // give time to send message
        resetFunc(); // call reset
    }
    else {
        printf("\r\nUnknown command: %s\r\n", buffer);
        printf("Use commands:\r\n");
        printf("  '" LED_ON_COMMAND "'\r\n");
        printf("  '" LED_OFF_COMMAND "'\r\n");
        printf("  '" LED_TOGGLE_COMMAND "'\r\n");
        printf("  '" IS_LED_ON_COMMAND "'\r\n");
        printf("  '" EXIT_COMMAND "'\r\n");
    }
    
}
#endif