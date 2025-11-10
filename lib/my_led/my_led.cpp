#include "my_led.h"

void led_init(Led* led, int pin) {
    led->ledPin = pin;
    led->ledState = false;
    pinMode(led->ledPin, OUTPUT);
    digitalWrite(led->ledPin, LOW);
}

void led_turn_on(Led* led) {
    digitalWrite(led->ledPin, HIGH);
    led->ledState = true;
}

void led_turn_off(Led* led) {
    digitalWrite(led->ledPin, LOW);
    led->ledState = false;
}

void led_toggle(Led* led) {
    if (led->ledState) {
        led_turn_off(led);
    } else {
        led_turn_on(led);
    }
}

void led_set_state(Led* led, bool state) {
    if (state) {
        led_turn_on(led);
    } else {
        led_turn_off(led);
    }
}

bool led_is_on(const Led* led) {
    return led->ledState;
}
