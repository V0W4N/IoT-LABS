#ifndef SERIAL_STDIO_H
#define SERIAL_STDIO_H

#include <Arduino.h>

// Initialize stdio redirection to Serial
void initSerialStdio(unsigned long baudRate);

// Custom putchar function for stdio
int serialPutchar(char c, FILE *file);

// Custom getchar function for stdio
int serialGetchar(FILE *file);

#endif