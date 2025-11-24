#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include <Arduino.h>
#include <stdint.h>

#define MAX_COMMANDS 16
#define MAX_COMMAND_NAME_LENGTH 32

// Command handler callback function type
// Returns true if command was handled successfully
// args can be nullptr if no arguments provided
typedef bool (*CommandCallback)(void* context, const char* args);

struct CommandEntry {
    char name[MAX_COMMAND_NAME_LENGTH];
    CommandCallback callback;
    void* context;
    const char* description;
};

#define COMMAND_BUFFER_SIZE 64

struct CommandHandler {
    CommandEntry commands[MAX_COMMANDS];
    uint8_t commandCount;
    CommandCallback defaultCallback;  // Called for unknown commands
    void* defaultContext;
    
    // Character accumulation buffer (internal use)
    char buffer[COMMAND_BUFFER_SIZE];
    size_t bufferIndex;
    bool commandReady;
};

/**
 * Initialize the command handler
 * @param handler Command handler instance
 * @param defaultCallback Callback for unknown commands (can be nullptr)
 * @param defaultContext Context for default callback
 */
void commandHandlerInit(CommandHandler* handler,
                        CommandCallback defaultCallback,
                        void* defaultContext);

/**
 * Register a command with the handler
 * @param handler Command handler instance
 * @param name Command name (case-insensitive, can contain spaces like "relay on")
 * @param callback Function to call when command is received
 * @param context User context passed to callback
 * @param description Help text for this command
 * @return true if command was registered successfully
 */
bool commandHandlerRegister(CommandHandler* handler,
                             const char* name,
                             CommandCallback callback,
                             void* context,
                             const char* description);

/**
 * Process a command string
 * @param handler Command handler instance
 * @param commandString Full command string (may include arguments after command name)
 * @return true if command was handled
 */
bool commandHandlerProcess(CommandHandler* handler, const char* commandString);

/**
 * Print help text for all registered commands
 */
void commandHandlerPrintHelp(CommandHandler* handler);

/**
 * Find command entry by name (case-insensitive)
 */
const CommandEntry* commandHandlerFindCommand(CommandHandler* handler, const char* name);

/**
 * Process a single character (accumulates until newline)
 * Call this for each character received from input
 * @param handler Command handler instance
 * @param c Character to process
 * @return true if a complete command was processed
 */
bool commandHandlerProcessChar(CommandHandler* handler, char c);

/**
 * Check if a command is ready to be processed
 */
bool commandHandlerIsReady(const CommandHandler* handler);

/**
 * Clear the command buffer
 */
void commandHandlerClear(CommandHandler* handler);

#endif

