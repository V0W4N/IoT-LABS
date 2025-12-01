#ifndef RTOS_BTN_H
#define RTOS_BTN_H

#include <Arduino.h>
#include <Arduino_FreeRTOS.h>
#include <semphr.h>
#include "my_btn.h"

/**
 * FreeRTOS-safe button wrapper
 * Provides thread-safe access to button state with mutex protection
 */
class RTOSButton {
private:
    ButtonUtils* button;
    SemaphoreHandle_t mutex;
    TaskHandle_t monitorTask;
    TickType_t updatePeriod;
    bool initialized;
    
    // Static task function for button monitoring
    static void monitorTaskFunction(void* pvParameters);

public:
    /**
     * Constructor
     * @param pin GPIO pin number
     * @param pullup Use internal pullup resistor (default: true)
     */
    explicit RTOSButton(int pin, bool pullup = true);
    
    /**
     * Destructor - cleans up RTOS resources
     */
    ~RTOSButton();
    
    /**
     * Start RTOS button monitoring task
     * @param updatePeriodMs Update period in milliseconds (default: 10ms)
     * @param priority Task priority (default: 2)
     * @return true if successfully started
     */
    bool start(TickType_t updatePeriodMs = pdMS_TO_TICKS(20), UBaseType_t priority = 2);
    
    /**
     * Stop RTOS button monitoring task
     */
    void stop();
    
    /**
     * Check if button is currently pressed (thread-safe)
     * @return true if button is pressed
     */
    bool isPressed();
    
    /**
     * Consume one button press (thread-safe, semaphore-like)
     * @return true if a press was consumed
     */
    bool consumePress();
    
    /**
     * Get number of pending presses (thread-safe)
     * @return Number of unconsumed presses
     */
    int getPressCount();
    
    /**
     * Reset press counter (thread-safe)
     */
    void resetPressCount();
    
    /**
     * Set debounce delay
     * @param delayMs Debounce delay in milliseconds
     */
    void setDebounceDelay(unsigned long delayMs);
    
    /**
     * Get underlying ButtonUtils instance (use with caution)
     * @return Pointer to ButtonUtils instance
     */
    ButtonUtils* getButton() { return button; }
};

#endif // RTOS_BTN_H

