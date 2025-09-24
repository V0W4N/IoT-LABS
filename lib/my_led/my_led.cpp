#include "my_led.h"


LedUtils::LedUtils(int pin) :
ledPin(pin) {
    ledState = false;
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
}

void LedUtils::turnOn() {
    digitalWrite(ledPin, HIGH);
    ledState = true;
}

void LedUtils::turnOff() {
    digitalWrite(ledPin, LOW);
    ledState = false;
}

void LedUtils::toggle() {
    if (ledState) 
        turnOff();
    else
        turnOn();
}

bool LedUtils::isOn() {
    return ledState;
}
