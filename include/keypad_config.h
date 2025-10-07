#ifndef KEYPAD_CONFIG_H
#define KEYPAD_CONFIG_H

#include <Arduino.h>

#define KEYPAD_ROWS 4
#define KEYPAD_COLS 4

const char KEYPAD_KEYS[KEYPAD_ROWS][KEYPAD_COLS] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};

const byte ROW_PINS[KEYPAD_ROWS] = {9, 8, 7, 6};
const byte COL_PINS[KEYPAD_COLS] = {5, 4, 3, 2};

#endif
