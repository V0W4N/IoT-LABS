#ifndef KEYPAD_LCD_STDIO_CONFIG_H
#define KEYPAD_LCD_STDIO_CONFIG_H

#include "keypad_lcd_stdio.h"
#include <keypad_config.h>
#include <lcd_config.h>
#include <Keypad.h>

inline void KeypadLCDStdio::initWithConfig() {
    // Use the config headers for initialization
    init(
        LCD_I2C_ADDR, LCD_COLS, LCD_ROWS,
        makeKeymap(KEYPAD_KEYS), 
        (byte*)ROW_PINS, 
        (byte*)COL_PINS, 
        KEYPAD_ROWS, 
        KEYPAD_COLS
    );
}

#endif

