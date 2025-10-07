#ifndef KEYPAD_STDIO_H
#define KEYPAD_STDIO_H

#include <Arduino.h>
#include <Keypad.h>

// Keypad stdio wrapper for character input
class KeypadStdio {
public:
    // Initialize the keypad
    static void init(char* keymap, byte* rowPins, byte* colPins, byte rows, byte cols);
    
    // Get a single character from keypad (for FILE stream)
    static int getcharkeypad();
    
    // Get keypad instance
    static Keypad* getKeypad();
    
    // Configuration
    static void setDebounceTime(uint debounce);
    static void setBlocking(bool blocking);  // Wait for key or return immediately
    
private:
    static Keypad* keypad;
    static bool blockingMode;  // If true, waits for key press
};

#endif

