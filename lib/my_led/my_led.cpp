#include "my_led.h"

// Initialize LED structure and hardware
void led_init(Led* led, int pin) {
    led->ledPin = pin;
    led->ledState = false;
    pinMode(led->ledPin, OUTPUT);
    digitalWrite(led->ledPin, LOW);
}

// Turn LED on
void led_turn_on(Led* led) {
    digitalWrite(led->ledPin, HIGH);
    led->ledState = true;
}

// Turn LED off
void led_turn_off(Led* led) {
    digitalWrite(led->ledPin, LOW);
    led->ledState = false;
}

// Toggle LED state
void led_toggle(Led* led) {
    if (led->ledState) {
        led_turn_off(led);
    } else {
        led_turn_on(led);
    }
}

// Set LED state directly
void led_set_state(Led* led, bool state) {
    if (state) {
        led_turn_on(led);
    } else {
        led_turn_off(led);
    }
}

// Check if LED is on
bool led_is_on(const Led* led) {
    return led->ledState;
}
