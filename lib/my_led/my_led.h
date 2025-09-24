#ifndef LED_UTILS_H
#define LED_UTILS_H

#include <Arduino.h>

class LedUtils {
public:
    explicit LedUtils(int pin);
    void turnOn();
    void turnOff();
    void toggle();
    bool isOn();
    
protected:
    int ledPin;
    bool ledState;
};

#endif
