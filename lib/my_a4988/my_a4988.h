#ifndef MY_A4988_H
#define MY_A4988_H

#include <Arduino.h>

/**
 * A4988 Stepper Motor Driver Module
 * 
 * Provides abstraction for controlling stepper motors via A4988 driver.
 * Uses a simplified DC-motor-like interface (power -100 to +100).
 * 
 * Pin connections:
 * - STEP: Step pulse pin (PWM capable for speed control)
 * - DIR: Direction control pin (digital)
 * - ENABLE: Enable pin (digital, active LOW)
 * 
 * Motor control:
 * - Positive power: forward direction (clockwise)
 * - Negative power: reverse direction (counter-clockwise)
 * - Zero power: stop
 * 
 * Speed control:
 * - Power magnitude determines step frequency (speed)
 * - Maximum frequency limited by motor and driver capabilities
 */

typedef struct {
    uint8_t stepPin;     // Step pulse pin (PWM)
    uint8_t dirPin;      // Direction control pin
    uint8_t enablePin;   // Enable pin (optional, can be 255 if not used)
    int8_t currentPower; // Current power setting (-100 to +100)
    bool isEnabled;      // Motor enable state
    
    // Internal timing (interrupt-based)
    uint16_t timerCompareValue;  // Timer compare value for interrupt (calculated from power)
    bool stepPinState;           // Current state of step pin (for toggling in ISR)
} A4988Motor;

/**
 * Initialize A4988 stepper driver
 * @param motor Motor driver instance
 * @param stepPin Step pulse pin (PWM capable)
 * @param dirPin Direction control pin
 * @param enablePin Enable pin (255 if not used)
 */
void a4988_init(A4988Motor* motor, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin);

/**
 * Set motor power (-100 to +100)
 * @param motor Motor driver instance
 * @param power Power level: -100 (max reverse) to +100 (max forward), 0 = stop
 */
void a4988_set_power(A4988Motor* motor, int8_t power);

/**
 * Stop motor immediately
 * @param motor Motor driver instance
 */
void a4988_stop(A4988Motor* motor);

/**
 * Get current power setting
 * @param motor Motor driver instance
 * @return Current power (-100 to +100)
 */
int8_t a4988_get_power(const A4988Motor* motor);

/**
 * Increase power by specified amount (clamped to ±100)
 * @param motor Motor driver instance
 * @param delta Power increment (typically 10)
 */
void a4988_increase_power(A4988Motor* motor, int8_t delta);

/**
 * Decrease power by specified amount (clamped to ±100)
 * @param motor Motor driver instance
 * @param delta Power decrement (typically 10)
 */
void a4988_decrease_power(A4988Motor* motor, int8_t delta);

/**
 * Set motor to maximum power in current direction
 * @param motor Motor driver instance
 */
void a4988_set_max(A4988Motor* motor);

/**
 * Check if motor is stopped
 * @param motor Motor driver instance
 * @return true if motor is stopped (power = 0)
 */
bool a4988_is_stopped(const A4988Motor* motor);

/**
 * Start interrupt-based motor control
 * Must be called after a4988_init() to enable timer interrupts
 */
void a4988_start_interrupts(void);

/**
 * Stop interrupt-based motor control
 * Disables timer interrupts
 */
void a4988_stop_interrupts(void);

/**
 * Internal ISR handler - do not call directly
 */
void a4988_isr_handler(void);

#endif

