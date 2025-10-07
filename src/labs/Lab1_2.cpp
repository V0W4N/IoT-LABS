#ifdef LAB_1_2
#include <Arduino.h>
#include "keypad_lcd_stdio_config.h"  // Includes initWithConfig() implementation
#include "my_led.h"
#include "config.h"

#define CORRECT_CODE "1234"
#define MAX_CODE_LENGTH 10

#define SECRET_INPUT  // To hide code input on LCD

// Lab 1.2: Code verification system with 4x4 keypad and LCD display via STDIO

LedUtils greenLed(GREEN_LED_PIN);
LedUtils redLed(RED_LED_PIN);

String enteredCode = "";

void resetBoard() {
    void(* resetFunc) (void) = 0; //declare reset function @ address 0
    resetFunc(); //call reset
}

void setup() {
    // Initialize STDIO wrapper for keypad and LCD display
    KeypadLCDStdio::initWithConfig();
    
    // Initialize LEDs
    greenLed.turnOff();
    redLed.turnOff();
    
    // Flush display first
    printf("\f");
    
    // Show first screen
    printf("Lab 1.2");
    printf("\n4-digit code");
    delay(2000);
    
    // Flush (clear) and show second screen
    printf("\f");
    printf("# to confirm");
    printf("\n* to clear");
    delay(2000);
    
    // Flush (clear) and show input prompt
    printf("\f");
    printf("Code: ");
}

void loop() {
    delay(10); // Small delay for stability
    
    // Read character directly from STDIO (blocking call)
    char c = getchar();
    
    if (c != EOF && c != '\0') {
        if (c == '#') {
            // Check code when # is pressed
            printf("\f");
            printf("Checking code...");
            delay(1000);
            
            if (enteredCode == CORRECT_CODE) {
                printf("\f");
                printf("Access granted!");
                greenLed.turnOn();
                redLed.turnOff();
            } else {
                printf("\f");
                printf("Access denied!");
                redLed.turnOn();
                greenLed.turnOff();
            }
            
            // Reset after 3 seconds
            delay(3000);
            printf("\f");
            resetBoard();

            
        } else if (c == '*') {
            // Clear input when * is pressed
            enteredCode = "";
            printf("\f");
            printf("Code cleared");
            delay(1000);
            printf("\f");
            printf("Code: ");
            
        } else if (c >= '0' && c <= '9') {
            // Add digit to code
            if (enteredCode.length() < MAX_CODE_LENGTH) {
                enteredCode += c;
            }
            /*
#ifdef SECRET_INPUT
            if (enteredCode.length() < MAX_CODE_LENGTH) {
                enteredCode += c;
                printf("\f");
                printf("Code: ");
                for (size_t i = 0; i < enteredCode.length(); i++) {
                    printf("*");
                }
            }
#else
            if (enteredCode.length() < MAX_CODE_LENGTH) {
                enteredCode += c;
                printf("\f");
                printf("Code: %s", enteredCode.c_str());
            }
#endif
*/
        }
    }
}
#endif
