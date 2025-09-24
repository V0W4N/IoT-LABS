#include "interactive_scanf.h"

bool exitSymbol(char ch, const char* exitChars);


void interactiveScanf(char* returnBuffer, int bufferSize, const char* intitMessage, const char* exitMessage,const char* eofMessage,const char* exitChars) 
{
    printf(intitMessage); // Print initial message
    int ch; // Variable to hold each character input
    for (;bufferSize > 1; bufferSize--) { // Leave space for null terminator
        ch = getchar(); // Read a character from input
        if (ch == EOF) { // Handle EOF (e.g., Ctrl+D)
            printf(eofMessage);
            break;
        }
        // Check for exit characters
        if (exitSymbol((char)ch, exitChars))
            break;

        *returnBuffer++ = (char)ch; // Store character in buffer
        printf("%c", ch); // Echo current character
    }
    fflush(stdin); // Clear input buffer to avoid residual characters
    *returnBuffer = '\0'; // Null-terminate the string
    printf(exitMessage); // Print exit message
}

inline bool exitSymbol(char ch, const char* exitChars) {
    // Check if the character is in the set of exit characters
    for(const char* p = exitChars; *p != '\0'; p++) 
        if (ch == *p)
            return true;
    
    return false;
}