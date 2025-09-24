#ifndef SERIAL_STDIO_H
#define SERIAL_STDIO_H

#include <Arduino.h>

// Utility functions for non-blocking delays using millis()


// Usage: accumulator = executePeriodically(accumulator, millisInterval  someFunction); // Calls someFunction every N milliseconds
// Use inside loop()
// Returns updated accumulator
int executePeriodically(const int acc, const int intervalMs, void (*func)());

// Usage: executePeriodicallyRef(accumulator, millisInterval  someFunction); // Calls someFunction every N milliseconds
// Use inside loop()
void executePeriodicallyRef(int& acc, const int intervalMs, void (*func)());



#endif