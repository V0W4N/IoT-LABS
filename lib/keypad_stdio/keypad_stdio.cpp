#include "keypad_stdio.h"

Keypad* KeypadStdio::keypad = nullptr;
bool KeypadStdio::blockingMode = true;  // Block by default

void KeypadStdio::init(char* keymap, byte* rowPins, byte* colPins, byte rows, byte cols) {
    keypad = new Keypad(keymap, rowPins, colPins, rows, cols);
}

Keypad* KeypadStdio::getKeypad() {
    return keypad;
}

void KeypadStdio::setDebounceTime(uint debounce) {
    if (keypad) {
        keypad->setDebounceTime(debounce);
    }
}

void KeypadStdio::setBlocking(bool blocking) {
    blockingMode = blocking;
}

int KeypadStdio::getcharkeypad() {
    if (!keypad) return EOF;
    
    char key = NO_KEY;
    
    if (blockingMode) {
        // Wait for a key press (blocking)
        while (key == NO_KEY) {
            key = keypad->getKey();
            delay(10); 
        }
    } else {
        key = keypad->getKey();
        if (key == NO_KEY) {
            return EOF;  // No key available
        }
    }
    
    return (int)key;
}

