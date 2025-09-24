#ifndef LCD_CONFIG_H
#define LCD_CONFIG_H

#include <LiquidCrystal_I2C.h>

#define LCD_I2C_ADDR 0x27  // I2C address of the LCD
#define LCD_COLS 16        // Number of columns
#define LCD_ROWS 2         // Number of rows

class LCDConfig {
public:
    static LiquidCrystal_I2C* initLCD() {
        LiquidCrystal_I2C* lcd = new LiquidCrystal_I2C(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
        lcd->init();
        lcd->backlight();
        lcd->clear();
        return lcd;
    }

    static void displayMessage(LiquidCrystal_I2C* lcd, const char* message, uint8_t row) {
        lcd->setCursor(0, row);
        lcd->print("                "); // Clear the line first
        lcd->setCursor(0, row);
        lcd->print(message);
    }

    static void clearDisplay(LiquidCrystal_I2C* lcd) {
        lcd->clear();
    }

    static void cleanup(LiquidCrystal_I2C* lcd) {
        if (lcd != nullptr) {
            lcd->clear();
            lcd->noBacklight();
        }
    }
};

#endif