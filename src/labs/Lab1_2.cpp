#ifdef LAB_1_2
#include <Arduino.h>
#include "lcd_config.h"
#include "keypad_config.h"
#include "my_led.h"
#include "config.h"

// Constants
#define CODE_LENGTH 4
#define CORRECT_CODE "1234"

// Global objects
LedUtils greenLed(GREEN_LED_PIN);
LedUtils redLed(RED_LED_PIN);
LiquidCrystal_I2C* lcd;
String enteredCode = "";

void(* resetFunc) (void) = 0;

void setup() {
    lcd = LCDConfig::initLCD();
    LCDConfig::displayMessage(lcd, "Enter code:", 0);
    
    greenLed.turnOff();
    redLed.turnOff();
}


void handleKeyPress(char key) {
    if (enteredCode.length() < CODE_LENGTH && isdigit(key)) {
        enteredCode += key;
        LCDConfig::displayMessage(lcd, enteredCode.c_str(), 1);
    } else if (!isdigit(key)) {
        // Ignore non-digit keys except * and #
        return;
    } else {
        // Code length exceeded
        LCDConfig::displayMessage(lcd, "Max length!", 1);
        delay(1000);
        LCDConfig::displayMessage(lcd, enteredCode.c_str(), 1);
    }
}

void resetInput() {
    enteredCode = "";
    LCDConfig::clearDisplay(lcd);
    LCDConfig::displayMessage(lcd, "Enter code:", 0);
    greenLed.turnOff();
    redLed.turnOff();
}

void resetSystem() {
    greenLed.turnOff();
    redLed.turnOff();
    LCDConfig::cleanup(lcd);
    resetFunc();
}


void validateCode() {
    bool isCorrect = (enteredCode == CORRECT_CODE);
    LCDConfig::clearDisplay(lcd);
    
    if (enteredCode.length() < CODE_LENGTH) {
        LCDConfig::displayMessage(lcd, "Code incomplete!", 0);
        delay(1000);
        resetInput();
        return;
    }
    
    LCDConfig::displayMessage(lcd, isCorrect ? "Correct!" : "Incorrect!", 0);
    
    if (isCorrect) {
        greenLed.turnOn();
        redLed.turnOff();
    } else {
        redLed.turnOn();
        greenLed.turnOff();
    }
    
    delay(2000);
    resetSystem();
}

void loop() {
    char key = getKeypad().getKey();
    
    if (key) {
        switch (key) {
            case '#':
                validateCode();
                break;
            case '*':
                resetInput();
                break;
            default:
                handleKeyPress(key);
                break;
        }
    }
    delay(50); // Debounce delay
}

#endif
