#ifndef SERIAL_STDIO_H
#define SERIAL_STDIO_H

#include <Arduino.h>

void initSerialStdio(unsigned long baudRate, bool echoEnabled = true);

void setSerialEcho(bool enabled);

int serialPutchar(char c, FILE *file);

int serialGetchar(FILE *file);

#endif