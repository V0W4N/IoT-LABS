#ifndef BUTTON_UTILS_H
#define BUTTON_UTILS_H

#include <Arduino.h>

class ButtonUtils {
protected:
    int buttonPin;
    bool pullupMode;
    bool lastState;
    bool currentState;

public:
    explicit ButtonUtils(int pin, bool pullup = true);
    bool isPressed();
    bool btnPressed();
    bool btnUnPressed();
    void update();
};

#endif
