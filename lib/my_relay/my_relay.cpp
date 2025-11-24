#include "my_relay.h"

void relay_init(Relay* relay, uint8_t pin) {
    relay->relayPin = pin;
    relay->relayState = false;
    pinMode(relay->relayPin, OUTPUT);
    digitalWrite(relay->relayPin, LOW);  // Relay off initially
}

void relay_turn_on(Relay* relay) {
    digitalWrite(relay->relayPin, HIGH);
    relay->relayState = true;
}

void relay_turn_off(Relay* relay) {
    digitalWrite(relay->relayPin, LOW);
    relay->relayState = false;
}

void relay_toggle(Relay* relay) {
    if (relay->relayState) {
        relay_turn_off(relay);
    } else {
        relay_turn_on(relay);
    }
}

void relay_set_state(Relay* relay, bool state) {
    if (state) {
        relay_turn_on(relay);
    } else {
        relay_turn_off(relay);
    }
}

bool relay_is_on(const Relay* relay) {
    return relay->relayState;
}

