#include "serial_stdio.h"

// Create a FILE structure to handle serial I/O
static FILE serialStdio;

int serialPutchar(char c, FILE *file) {
    Serial.write(c);
    return c;
}

int serialGetchar(FILE *file) {
    while (!Serial.available());
    return Serial.read();
}

void initSerialStdio(unsigned long baudRate) {
    Serial.begin(baudRate);
    
    // Set up new streams for stdin/stdout
    fdev_setup_stream(&serialStdio, serialPutchar, serialGetchar, _FDEV_SETUP_RW);
    
    // Redirect stdin/stdout/stderr to our new streams
    stdout = &serialStdio;
    stdin = &serialStdio;
    stderr = &serialStdio;
}