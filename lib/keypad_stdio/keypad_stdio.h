#ifndef KEYPAD_STDIO_H
#define KEYPAD_STDIO_H

#include <Arduino.h>
#include <Keypad.h>

class KeypadStdio {
public:
    static void init(char* keymap, byte* rowPins, byte* colPins, byte rows, byte cols);
    
    static int getcharkeypad();
    
    static Keypad* getKeypad();
    
    // Configuration
    static void setDebounceTime(uint debounce);
    static void setBlocking(bool blocking);  // Wait for key or return immediately
    
private:
    static Keypad* keypad;
    static bool blockingMode;
};

#endif

