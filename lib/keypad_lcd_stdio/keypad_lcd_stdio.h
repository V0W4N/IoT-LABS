#ifndef KEYPAD_LCD_STDIO_H
#define KEYPAD_LCD_STDIO_H

#include <Arduino.h>
#include "lcd_stdio.h"
#include "keypad_stdio.h"

// Combined keypad+LCD stdio redirection
// This provides a complete stdio replacement for Arduino
class KeypadLCDStdio {
public:
    // Initialize both LCD and Keypad, then redirect stdio
    static void init(
        // LCD parameters
        uint8_t lcdAddr, uint8_t lcdCols, uint8_t lcdRows,
        // Keypad parameters
        char* keymap, byte* rowPins, byte* colPins, byte keyRows, byte keyCols
    );
    
    // Alternative: use with existing config
    // Note: Implementation in header because it needs access to config files
    static void initWithConfig();
    
    // Access to underlying components
    static LiquidCrystal_I2C* getLCD();
    static Keypad* getKeypad();
    
    // Configuration
    static void setKeyEcho(bool echo);  // Echo keys to LCD
    static void setBlocking(bool blocking);  // Blocking input
    
private:
    // The FILE stream structure
    static FILE keypadLCDStream;
    
    // Stdio callbacks
    static int putcharCallback(char c, FILE* file);
    static int getcharCallback(FILE* file);
    
    // Configuration
    static bool echoKeys;
};

#endif

