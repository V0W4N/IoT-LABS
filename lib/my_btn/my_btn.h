#ifndef BUTTON_UTILS_H
#define BUTTON_UTILS_H

#include <Arduino.h>

/**
 * Simple button class with built-in debouncing
 * All state checking is done in a single function call
 */
class ButtonUtils {
private:
    int buttonPin;
    bool pullupMode;
    bool lastStableState;
    bool previousStableState;  // For edge detection
    volatile int pressCount;
    unsigned long lastDebounceTime;
    unsigned long debounceDelayMs;

public:
    /**
     * Constructor
     * @param pin GPIO pin number
     * @param pullup Use internal pullup resistor (default: true)
     */
    explicit ButtonUtils(int pin, bool pullup = true);
    
    /**
     * Check button state with automatic debouncing
     * Call this once per loop() - it handles everything internally
     * @return true if button is pressed (debounced)
     */
    bool checkState();
    
    /**
     * Check if button was just pressed (rising edge)
     * Must be called after checkState()
     * @return true on press event
     */
    bool wasPressed();
    
    /**
     * Check if button was just released (falling edge)
     * Must be called after checkState()
     * @return true on release event
     */
    bool wasReleased();

    /**
     * Consume one press from the counter (decrements count by 1)
     * @return true if a press was consumed (count was > 0)
     */
    bool consumePress();
    
    /**
     * Get current press count (non-consuming)
     * @return Current number of unprocessed presses
     */
    int getPressCount() const;
    
    /**
     * Reset press counter to zero
     */
    void resetPressCount();
    
    /**
     * Set debounce delay in milliseconds
     * @param delayMs Debounce delay (default: 50ms)
     */
    void setDebounceDelay(unsigned long delayMs);
    
    /**
     * Get current debounce delay
     * @return Debounce delay in milliseconds
     */
    unsigned long getDebounceDelay() const;
};

#endif
