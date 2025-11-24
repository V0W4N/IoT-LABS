#include "command_handler.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static int strcasecmp_arduino(const char* s1, const char* s2) {
    while (*s1 && *s2) {
        char c1 = tolower(*s1);
        char c2 = tolower(*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
    }
    return tolower(*s1) - tolower(*s2);
}

static int strncasecmp_arduino(const char* s1, const char* s2, size_t n) {
    size_t i = 0;
    while (i < n && *s1 && *s2) {
        char c1 = tolower(*s1);
        char c2 = tolower(*s2);
        if (c1 != c2) {
            return c1 - c2;
        }
        s1++;
        s2++;
        i++;
    }
    if (i >= n) {
        return 0;
    }
    return tolower(*s1) - tolower(*s2);
}

void commandHandlerInit(CommandHandler* handler,
                        CommandCallback defaultCallback,
                        void* defaultContext) {
    memset(handler->commands, 0, sizeof(handler->commands));
    handler->commandCount = 0;
    handler->defaultCallback = defaultCallback;
    handler->defaultContext = defaultContext;
    
    // Initialize character buffer
    memset(handler->buffer, 0, COMMAND_BUFFER_SIZE);
    handler->bufferIndex = 0;
    handler->commandReady = false;
}

bool commandHandlerRegister(CommandHandler* handler,
                             const char* name,
                             CommandCallback callback,
                             void* context,
                             const char* description) {
    if (handler->commandCount >= MAX_COMMANDS) {
        return false;
    }

    if (name == nullptr || callback == nullptr) {
        return false;
    }

    size_t nameLen = strlen(name);
    if (nameLen == 0 || nameLen >= MAX_COMMAND_NAME_LENGTH) {
        return false;
    }

    // Check for duplicate command names
    for (uint8_t i = 0; i < handler->commandCount; i++) {
        if (strcasecmp_arduino(handler->commands[i].name, name) == 0) {
            return false;  // Duplicate command
        }
    }

    // Add new command
    CommandEntry* entry = &handler->commands[handler->commandCount];
    strncpy(entry->name, name, MAX_COMMAND_NAME_LENGTH - 1);
    entry->name[MAX_COMMAND_NAME_LENGTH - 1] = '\0';
    entry->callback = callback;
    entry->context = context;
    entry->description = description ? description : "";

    handler->commandCount++;
    return true;
}

bool commandHandlerProcess(CommandHandler* handler, const char* commandString) {
    if (handler == nullptr || commandString == nullptr) {
        return false;
    }

    // Skip leading whitespace
    while (isspace(*commandString)) {
        commandString++;
    }

    if (*commandString == '\0') {
        return false;
    }

    // Try to match commands (supporting multi-word commands like "relay on")
    // We need to check if the command string starts with any registered command name
    const CommandEntry* bestMatch = nullptr;
    size_t bestMatchLen = 0;

    for (uint8_t j = 0; j < handler->commandCount; j++) {
        size_t cmdNameLen = strlen(handler->commands[j].name);
        
        // Check if command string starts with this command name
        if (strncasecmp_arduino(commandString, handler->commands[j].name, cmdNameLen) == 0) {
            // Check if it's a complete match (end of string or followed by space)
            char nextChar = commandString[cmdNameLen];
            if (nextChar == '\0' || isspace(nextChar)) {
                // This is a match, check if it's longer (more specific) than previous match
                if (cmdNameLen > bestMatchLen) {
                    bestMatch = &handler->commands[j];
                    bestMatchLen = cmdNameLen;
                }
            }
        }
    }

    if (bestMatch != nullptr) {
        // Extract arguments (everything after the command name)
        const char* args = commandString + bestMatchLen;
        while (isspace(*args)) {
            args++;
        }
        if (*args == '\0') {
            args = nullptr;
        }

        // Execute callback
        if (bestMatch->callback) {
            return bestMatch->callback(bestMatch->context, args);
        }
        return true;
    }

    // Command not found, call default callback if available
    if (handler->defaultCallback) {
        return handler->defaultCallback(handler->defaultContext, commandString);
    }

    return false;
}

bool commandHandlerProcessChar(CommandHandler* handler, char c) {
    if (handler == nullptr) {
        return false;
    }

    // If command was already processed, clear buffer
    if (handler->commandReady) {
        commandHandlerClear(handler);
    }

    // Handle newline/carriage return - command complete
    if (c == '\r' || c == '\n') {
        if (handler->bufferIndex > 0) {
            handler->buffer[handler->bufferIndex] = '\0';
            handler->commandReady = true;
            
            // Process the command immediately
            bool result = commandHandlerProcess(handler, handler->buffer);
            commandHandlerClear(handler);
            return result;
        }
        return false;
    }

    // Handle backspace/delete
    if (c == '\b' || c == 127) {
        if (handler->bufferIndex > 0) {
            handler->bufferIndex--;
            handler->buffer[handler->bufferIndex] = '\0';
        }
        return false;
    }

    // Add printable character to buffer
    if (handler->bufferIndex < COMMAND_BUFFER_SIZE - 1 && isprint(c)) {
        handler->buffer[handler->bufferIndex++] = tolower(c);
        handler->buffer[handler->bufferIndex] = '\0';
    }

    return false;
}

bool commandHandlerIsReady(const CommandHandler* handler) {
    if (handler == nullptr) {
        return false;
    }
    return handler->commandReady;
}

void commandHandlerClear(CommandHandler* handler) {
    if (handler == nullptr) {
        return;
    }
    memset(handler->buffer, 0, COMMAND_BUFFER_SIZE);
    handler->bufferIndex = 0;
    handler->commandReady = false;
}

void commandHandlerPrintHelp(CommandHandler* handler) {
    if (handler == nullptr) {
        return;
    }

    printf("\r\nAvailable commands:\r\n");
    for (uint8_t i = 0; i < handler->commandCount; i++) {
        printf("  %s", handler->commands[i].name);
        if (handler->commands[i].description && strlen(handler->commands[i].description) > 0) {
            printf(" - %s", handler->commands[i].description);
        }
        printf("\r\n");
    }
    printf("\r\n");
}

const CommandEntry* commandHandlerFindCommand(CommandHandler* handler, const char* name) {
    if (handler == nullptr || name == nullptr) {
        return nullptr;
    }

    for (uint8_t i = 0; i < handler->commandCount; i++) {
        if (strcasecmp_arduino(handler->commands[i].name, name) == 0) {
            return &handler->commands[i];
        }
    }

    return nullptr;
}

