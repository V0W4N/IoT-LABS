#ifndef BUTTON_UTILS_H
#define BUTTON_UTILS_H

#include <Arduino.h>

class ButtonUtils {
protected:
    const int buttonPin;
    bool lastState;
    bool currentState;
    const bool pullupMode;

public:
    explicit ButtonUtils(const int pin, const bool pullup = true);
    bool isPressed();
    bool btnPressed();
    bool btnUnPressed();
    void update();
};

#endif
