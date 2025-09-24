#ifndef INTERACTIVE_SCANF_H
#define INTERACTIVE_SCANF_H

#include <Arduino.h>

void interactiveScanf(
        char* returnBuffer, int bufferSize, 
        const char* intitMessage = "Type characters (press [Enter] to exit):\n", 
        const char* exitMessage = "\n",
        const char* eofMessage = "WARNING: Unexpected exit...\n",
        const char* exitChars = "\r\n");

#endif // INTERACTIVE_SCANF_H