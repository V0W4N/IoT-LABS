#ifndef MY_RELAY_H
#define MY_RELAY_H

#include <Arduino.h>

typedef struct {
    uint8_t relayPin;
    bool relayState;
} Relay;

void relay_init(Relay* relay, uint8_t pin);
void relay_turn_on(Relay* relay);
void relay_turn_off(Relay* relay);
void relay_toggle(Relay* relay);
void relay_set_state(Relay* relay, bool state);
bool relay_is_on(const Relay* relay);

#endif

