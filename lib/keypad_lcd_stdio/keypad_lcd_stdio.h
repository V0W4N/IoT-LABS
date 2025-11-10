#ifndef KEYPAD_LCD_STDIO_H
#define KEYPAD_LCD_STDIO_H

#include <Arduino.h>
#include "lcd_stdio.h"
#include "keypad_stdio.h"

class KeypadLCDStdio {
public:
    static void init(
        // LCD parameters
        uint8_t lcdAddr, uint8_t lcdCols, uint8_t lcdRows,
        // Keypad parameters
        char* keymap, byte* rowPins, byte* colPins, byte keyRows, byte keyCols
    );
    
    static void initWithConfig();
    
    static LiquidCrystal_I2C* getLCD();
    static Keypad* getKeypad();
    
    // Configuration
    static void setKeyEcho(bool echo);  // Echo keys to LCD
    static void setBlocking(bool blocking);  // Blocking input
    
private:
    static FILE keypadLCDStream;
    
    static int putcharCallback(char c, FILE* file);
    static int getcharCallback(FILE* file);
    
    static bool echoKeys;
};

#endif

