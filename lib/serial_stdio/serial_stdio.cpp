#include "serial_stdio.h"

static FILE serialStdio;
static bool echoEnabled = true;

int serialPutchar(char c, FILE *file) {
    Serial.write(c);
    return c;
}

int serialGetchar(FILE *file) {
    while (!Serial.available());
    int c = Serial.read();
    
    // Echo character back if echo is enabled
    if (echoEnabled && c != EOF) {
        // Handle backspace/delete specially for proper visual erasure
        if (c == '\b' || c == 127) {
            // Send backspace-space-backspace sequence to erase character visually
            // This is standard terminal behavior:
            // 1. Backspace moves cursor back
            // 2. Space erases the character
            // 3. Backspace moves cursor back again to correct position
            Serial.write('\b');
            Serial.write(' ');
            Serial.write('\b');
        } else {
            // Echo regular characters normally
            Serial.write(c);
        }
    }
    
    return c;
}

void initSerialStdio(unsigned long baudRate, bool echoEnabledParam) {
    Serial.begin(baudRate);
    echoEnabled = echoEnabledParam;
    
    // Set up new streams for stdin/stdout
    fdev_setup_stream(&serialStdio, serialPutchar, serialGetchar, _FDEV_SETUP_RW);
    
    // Redirect stdin/stdout/stderr to our new streams
    stdout = &serialStdio;
    stdin = &serialStdio;
    stderr = &serialStdio;
}

void setSerialEcho(bool enabled) {
    echoEnabled = enabled;
}