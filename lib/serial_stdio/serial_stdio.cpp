#include "serial_stdio.h"

// Create a FILE structure to handle serial I/O
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
        Serial.write(c);
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