#ifdef LAB_1_2
#include <Arduino.h>
#include "utils/led_utils.h"
#include "sensors/keypad_sensor.h"
#include "configs/pin_config.h"
#include "configs/system_config.h"
#include "configs/lcd_config.h"

// Lab 1.2: Система проверки кода с клавиатуры 4x4 и ЖК-дисплеем

// Пины для клавиатуры 4x4
int keypadRowPins[] = {3, 4, 5, 6};
int keypadColPins[] = {7, 8, 9, 10};

LedUtils greenLed(GREEN_LED_PIN);
LedUtils redLed(RED_LED_PIN);
KeypadSensor keypad(keypadRowPins, keypadColPins);
LiquidCrystal_I2C* lcd;

String enteredCode = "";
bool codeEntered = false;

void setup() {
    lcd = LCDConfig::initLCD();
    
    LCDConfig::displayMessage(lcd, WELCOME_MSG, 0);
    LCDConfig::showCursor(lcd, true);
    
    // Инициализация светодиодов
    greenLed.turnOff();
    redLed.turnOff();
}

void loop() {
    char key = keypad.getKey();
    
    if (key != '\0') {
        if (key == '#') {
            // Проверка кода
            checkCode();
        }
        else if (key == '*') {
            // Очистка ввода
            clearInput();
        }
        else if (key >= '0' && key <= '9') {
            // Добавление цифры к коду
            if (enteredCode.length() < MAX_CODE_LENGTH) {
                enteredCode += key;
                updateDisplay();
            }
        }
        
        delay(DEBOUNCE_DELAY);
    }
    
    delay(SCAN_DELAY);
}

void checkCode() {
    LCDConfig::clearDisplay(lcd);
    
    if (enteredCode == CORRECT_CODE) {
        LCDConfig::displayMessage(lcd, CORRECT_MSG, 0);
        greenLed.turnOn();
        redLed.turnOff();
    } else {
        LCDConfig::displayMessage(lcd, INCORRECT_MSG, 0);
        redLed.turnOn();
        greenLed.turnOff();
    }
    
    // Сброс через 3 секунды
    delay(3000);
    resetSystem();
}

void clearInput() {
    enteredCode = "";
    updateDisplay();
}

void updateDisplay() {
    LCDConfig::clearDisplay(lcd);
    LCDConfig::displayMessage(lcd, WELCOME_MSG, 0);
    LCDConfig::displayMessage(lcd, enteredCode, 1);
    LCDConfig::showCursor(lcd, true);
}

void resetSystem() {
    enteredCode = "";
    greenLed.turnOff();
    redLed.turnOff();
    LCDConfig::clearDisplay(lcd);
    LCDConfig::displayMessage(lcd, WELCOME_MSG, 0);
    LCDConfig::showCursor(lcd, true);
}
#endif
