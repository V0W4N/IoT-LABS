#include "lcd_stdio.h"

// Static member initialization
LiquidCrystal_I2C* LCDStdio::lcd = nullptr;
uint8_t LCDStdio::lcdCols = 16;
uint8_t LCDStdio::lcdRows = 2;
uint8_t LCDStdio::cursorCol = 0;
uint8_t LCDStdio::cursorRow = 0;

LiquidCrystal_I2C* LCDStdio::init(uint8_t addr, uint8_t cols, uint8_t rows) {
    lcdCols = cols;
    lcdRows = rows;
    cursorCol = 0;
    cursorRow = 0;
    
    // Create LCD instance
    lcd = new LiquidCrystal_I2C(addr, cols, rows);
    lcd->init();
    lcd->backlight();
    lcd->clear();
    lcd->setCursor(0, 0);
    return lcd;
}

LiquidCrystal_I2C* LCDStdio::getLCD() {
    return lcd;
}

void LCDStdio::setCursor(uint8_t col, uint8_t row) {
    if (col < lcdCols && row < lcdRows) {
        cursorCol = col;
        cursorRow = row;
        if (lcd) {
            lcd->setCursor(cursorCol, cursorRow);
        }
    }
}

void LCDStdio::getCursor(uint8_t& col, uint8_t& row) {
    col = cursorCol;
    row = cursorRow;
}

void LCDStdio::clear() {
    if (lcd) {
        lcd->clear();
        cursorCol = 0;
        cursorRow = 0;
    }
}

int LCDStdio::putcharlcd(char c, FILE* file) {
    if (!lcd) return -1;
    
    switch (c) {
        case '\n':
            handleNewline();
            break;
            
        case '\r':
            handleCarriageReturn();
            break;
            
        case '\f':
            handleFormFeed();
            break;
            
        case '\t':
            handleTab();
            break;
            
        case '\b':
            if (cursorCol > 0) {
                cursorCol--;
                lcd->setCursor(cursorCol, cursorRow);
                lcd->print(' ');  // Erase character
                lcd->setCursor(cursorCol, cursorRow);
            }
            break;
            
        default:  // Regular character
            if (c >= 32 && c <= 126) {  // Printable ASCII
                printCharacter(c);
            }
            break;
    }
    
    return c;
}

void LCDStdio::handleNewline() {
    cursorCol = 0;
    if (cursorRow + 1 < lcdRows) {
        cursorRow++;
    } else {
        // At bottom - scroll up or wrap to top
        scrollUp();
    }
    lcd->setCursor(cursorCol, cursorRow);
}

void LCDStdio::handleCarriageReturn() {
    cursorCol = 0;
    lcd->setCursor(cursorCol, cursorRow);
}

void LCDStdio::handleFormFeed() {
    lcd->clear();
    cursorCol = 0;
    cursorRow = 0;
    lcd->setCursor(0, 0);
}

void LCDStdio::handleTab() {
    // Tab = 4 spaces, aligned to multiples of 4
    uint8_t spaces = 4 - (cursorCol % 4);
    for (uint8_t i = 0; i < spaces; i++) {
        printCharacter(' ');
    }
}

void LCDStdio::printCharacter(char c) {
    // Check if we need to wrap
    if (cursorCol >= lcdCols) {
        cursorCol = 0;
        if (cursorRow + 1 < lcdRows) {
            cursorRow++;
        } else {
            scrollUp();
        }
    }
    
    // Print character at current position
    lcd->setCursor(cursorCol, cursorRow);
    lcd->print(c);
    
    // Advance cursor
    cursorCol++;
}

void LCDStdio::advanceCursor() {
    cursorCol++;
    if (cursorCol >= lcdCols) {
        cursorCol = 0;
        if (cursorRow + 1 < lcdRows) {
            cursorRow++;
        } else {
            scrollUp();
        }
        lcd->setCursor(cursorCol, cursorRow);
    }
}

void LCDStdio::scrollUp() {
    lcd->clear();
    cursorRow = 0;
    cursorCol = 0;
    lcd->setCursor(0, 0);
}

