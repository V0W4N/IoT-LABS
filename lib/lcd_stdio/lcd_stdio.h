#ifndef LCD_STDIO_H
#define LCD_STDIO_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

// LCD stdio wrapper for character output
class LCDStdio {
public:
    // Initialize the LCD
    static LiquidCrystal_I2C* init(uint8_t addr, uint8_t cols, uint8_t rows);
    
    // Put a single character to LCD (for FILE stream)
    static int putcharlcd(char c, FILE* file);
    
    // Get LCD instance
    static LiquidCrystal_I2C* getLCD();
    
    // Position management
    static void setCursor(uint8_t col, uint8_t row);
    static void getCursor(uint8_t& col, uint8_t& row);
    static void clear();
    
private:
    static LiquidCrystal_I2C* lcd;
    static uint8_t lcdCols;
    static uint8_t lcdRows;
    static uint8_t cursorCol;
    static uint8_t cursorRow;
    
    // Helper functions
    static void handleNewline();
    static void handleCarriageReturn();
    static void handleFormFeed();
    static void handleTab();
    static void printCharacter(char c);
    static void advanceCursor();
    static void scrollUp();
};

#endif

