#ifndef LED_UTILS_H
#define LED_UTILS_H

#include <Arduino.h>

#ifdef __cplusplus
extern "C" {
#endif

// LED structure (C-style)
typedef struct {
    int ledPin;
    bool ledState;
} Led;

// LED function prototypes
void led_init(Led* led, int pin);
void led_turn_on(Led* led);
void led_turn_off(Led* led);
void led_toggle(Led* led);
void led_set_state(Led* led, bool state);
bool led_is_on(const Led* led);

#ifdef __cplusplus
}
#endif

#endif
