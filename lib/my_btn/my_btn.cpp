#include "my_btn.h"

ButtonUtils::ButtonUtils(int pin, bool pullup)
    : buttonPin(pin), pullupMode(pullup), debounceDelayMs(50) {
    
    // Initialize pin
    if (pullup) 
        pinMode(buttonPin, INPUT_PULLUP);
    else
        pinMode(buttonPin, INPUT);
    
    // Read initial state
    bool rawState = digitalRead(buttonPin);
    if (pullupMode) 
        rawState = !rawState;
    
    pressCount = 0;
    lastStableState = rawState;
    previousStableState = rawState;
    lastDebounceTime = millis();
}

bool ButtonUtils::checkState() {
    // Read current raw state
    bool rawState = digitalRead(buttonPin);
    if (pullupMode) 
        rawState = !rawState;
    
    unsigned long currentTime = millis();
    
    // If stable for debounce period, update state
    if ((currentTime - lastDebounceTime) >= debounceDelayMs && rawState != lastStableState) {

        previousStableState = lastStableState;
        pressCount += (rawState == true) ? 1 : 0;
        lastDebounceTime = currentTime;
        lastStableState = rawState;
    }
    
    return lastStableState;
}

bool ButtonUtils::consumePress() {
    if (pressCount > 0) {
        pressCount--;
        return true;
    }
    return false;
}

int ButtonUtils::getPressCount() const {
    return pressCount;
}

void ButtonUtils::resetPressCount() {
    pressCount = 0;
}

bool ButtonUtils::wasPressed() {
    return !previousStableState && lastStableState;
}

bool ButtonUtils::wasReleased() {
    return previousStableState && !lastStableState;
}

void ButtonUtils::setDebounceDelay(unsigned long delayMs) {
    debounceDelayMs = delayMs;
}

unsigned long ButtonUtils::getDebounceDelay() const {
    return debounceDelayMs;
}
