#include "keypad_lcd_stdio.h"

// Static member initialization
FILE KeypadLCDStdio::keypadLCDStream;
bool KeypadLCDStdio::echoKeys = true;  // Echo by default

void KeypadLCDStdio::init(
    uint8_t lcdAddr, uint8_t lcdCols, uint8_t lcdRows,
    char* keymap, byte* rowPins, byte* colPins, byte keyRows, byte keyCols
) {
    // Initialize LCD
    LCDStdio::init(lcdAddr, lcdCols, lcdRows);
    
    // Initialize Keypad
    KeypadStdio::init(keymap, rowPins, colPins, keyRows, keyCols);
    
    // Set up the FILE stream with custom put/get functions
    fdev_setup_stream(&keypadLCDStream, putcharCallback, getcharCallback, _FDEV_SETUP_RW);
    
    // Redirect stdin/stdout/stderr to custom stream
    stdout = &keypadLCDStream;
    stdin = &keypadLCDStream;
    stderr = &keypadLCDStream;
}

LiquidCrystal_I2C* KeypadLCDStdio::getLCD() {
    return LCDStdio::getLCD();
}

Keypad* KeypadLCDStdio::getKeypad() {
    return KeypadStdio::getKeypad();
}

void KeypadLCDStdio::setKeyEcho(bool echo) {
    echoKeys = echo;
}

void KeypadLCDStdio::setBlocking(bool blocking) {
    KeypadStdio::setBlocking(blocking);
}

int KeypadLCDStdio::putcharCallback(char c, FILE* file) {
    // Delegate to LCD stdio
    return LCDStdio::putcharlcd(c, file);
}

int KeypadLCDStdio::getcharCallback(FILE* file) {
    // Read character from keypad
    int c = KeypadStdio::getcharkeypad();
    
    // Echo to LCD if enabled
    if (c != EOF && echoKeys) {
        LCDStdio::putcharlcd((char)c, file);
    }
    
    return c;
}

