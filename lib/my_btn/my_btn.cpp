#include "my_btn.h"

ButtonUtils::ButtonUtils(int pin, const bool pullup)
: pullupMode(pullup), buttonPin(pin) {
    lastState       = pullup ? true : false;
    currentState    = pullup ? true : false;
    
    if (pullup) 
        pinMode(buttonPin, INPUT_PULLUP);
    else
        pinMode(buttonPin, INPUT);
}

bool ButtonUtils::isPressed() {
    return currentState;
}

bool ButtonUtils::btnPressed() {
    return !lastState && currentState ;
}

bool ButtonUtils::btnUnPressed()
{
    return lastState && !currentState;
}

void ButtonUtils::update() {
    lastState = currentState;

    currentState = digitalRead(buttonPin);
    
    if (pullupMode) 
        currentState = !currentState;
}
