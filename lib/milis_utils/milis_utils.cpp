#include "milis_utils.h"

int executePeriodically(const int acc, const int intervalMs, void (*func)()) {
    int mil = millis();
    if (acc < mil) {
        func();
        return mil + intervalMs;
    }
    return acc;
}

void executePeriodicallyRef(int& acc, const int intervalMs, void (*func)()) {
    int mil = millis();
    if (acc < mil) {
        func();
        acc = mil + intervalMs;
    }
}